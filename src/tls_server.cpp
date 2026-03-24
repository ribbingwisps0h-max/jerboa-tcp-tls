// ─────────────────────────────────────────────────────────────────────────────
//  jerboa-tcp-tls :: tls_server.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include "tls_server.hpp"
#include "logger.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/system_error.hpp>

#include <csignal>
#include <expected>
#include <format>
#include <string>

// OpenSSL
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace jerboa {

// ── ctor ──────────────────────────────────────────────────────────────────────
TlsServer::TlsServer(const Config& cfg)
    : cfg_(cfg)
    , ioc_(1)                           // single-threaded executor
    , ssl_ctx_(ssl::context::tls_client) // outbound: we are the TLS client
    , acceptor_(ioc_) {}

// ── init ──────────────────────────────────────────────────────────────────────
std::expected<void, std::string> TlsServer::init() {
    boost::system::error_code ec;

    // ── SSL context hardening ─────────────────────────────────────────────────
    ssl_ctx_.set_options(
        ssl::context::no_sslv2   |
        ssl::context::no_sslv3   |
        ssl::context::no_tlsv1   |
        ssl::context::no_tlsv1_1 |
        ssl::context::no_tlsv1_2 |   // TLS 1.3 only
        ssl::context::single_dh_use);

    // Force TLS 1.3 via native OpenSSL API
    SSL_CTX_set_min_proto_version(ssl_ctx_.native_handle(), TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ssl_ctx_.native_handle(), TLS1_3_VERSION);

    // Modern TLS 1.3 cipher suites (OpenSSL default is already good;
    // be explicit for documentation purposes)
    SSL_CTX_set_ciphersuites(ssl_ctx_.native_handle(),
        "TLS_AES_256_GCM_SHA384:"
        "TLS_CHACHA20_POLY1305_SHA256:"
        "TLS_AES_128_GCM_SHA256");

    // ── Peer verification ─────────────────────────────────────────────────────
    if (cfg_.verify_peer) {
        ssl_ctx_.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);

        if (!cfg_.ca_file.empty()) {
            ssl_ctx_.load_verify_file(cfg_.ca_file, ec);
            if (ec) {
                return std::unexpected(
                    std::format("Failed to load CA file '{}': {}",
                                cfg_.ca_file, ec.message()));
            }
            LOG_INFO("Loaded CA bundle: {}", cfg_.ca_file);
        } else {
            // Fall back to system CA store
            ssl_ctx_.set_default_verify_paths(ec);
            if (ec) {
                return std::unexpected(
                    std::format("set_default_verify_paths: {}", ec.message()));
            }
            LOG_INFO("Using system CA store for peer verification");
        }
    } else {
        ssl_ctx_.set_verify_mode(ssl::verify_none);
        LOG_WARN("Peer certificate verification DISABLED");
    }

    // ── mTLS client certificate ───────────────────────────────────────────────
    if (cfg_.mtls) {
        if (cfg_.cert_file.empty() || cfg_.key_file.empty()) {
            return std::unexpected(
                "mTLS requires both --cert and --key to be specified");
        }
        ssl_ctx_.use_certificate_chain_file(cfg_.cert_file, ec);
        if (ec) {
            return std::unexpected(
                std::format("use_certificate_chain_file '{}': {}",
                            cfg_.cert_file, ec.message()));
        }
        ssl_ctx_.use_private_key_file(cfg_.key_file, ssl::context::pem, ec);
        if (ec) {
            return std::unexpected(
                std::format("use_private_key_file '{}': {}",
                            cfg_.key_file, ec.message()));
        }
        LOG_INFO("mTLS enabled  cert={} key={}", cfg_.cert_file, cfg_.key_file);
    }

    // ── TCP acceptor setup ────────────────────────────────────────────────────
    tcp::endpoint endpoint(tcp::v4(), cfg_.listen_port);
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) return std::unexpected(std::format("acceptor open: {}", ec.message()));

    acceptor_.set_option(tcp::acceptor::reuse_address(true), ec);
    if (ec) return std::unexpected(std::format("reuse_address: {}", ec.message()));

    acceptor_.bind(endpoint, ec);
    if (ec) return std::unexpected(std::format("bind :{}: {}", cfg_.listen_port, ec.message()));

    acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) return std::unexpected(std::format("listen: {}", ec.message()));

    LOG_INFO("Listening on 0.0.0.0:{}", cfg_.listen_port);
    LOG_INFO("Forwarding to {}:{}", cfg_.remote_host, cfg_.remote_port);

    return {};  // std::expected<void, string> success
}

// ── run ───────────────────────────────────────────────────────────────────────
void TlsServer::run() {
    // Graceful shutdown on SIGINT / SIGTERM
    asio::signal_set signals(ioc_, SIGINT, SIGTERM);
    signals.async_wait([this](auto /*ec*/, int sig) {
        LOG_INFO("Received signal {} — shutting down...", sig);
        ioc_.stop();
    });

    asio::co_spawn(ioc_, accept_loop(), asio::detached);

    LOG_INFO("jerboa-tcp-tls running (Ctrl-C to stop)");
    ioc_.run();
    LOG_INFO("jerboa-tcp-tls stopped");
}

// ── accept_loop ───────────────────────────────────────────────────────────────
asio::awaitable<void> TlsServer::accept_loop() {
    for (;;) {
        boost::system::error_code ec;
        tcp::socket sock(ioc_);

        co_await acceptor_.async_accept(sock, asio::redirect_error(asio::use_awaitable, ec));

        if (ec == asio::error::operation_aborted) {
            // io_context stopped
            break;
        }
        if (ec) {
            LOG_WARN("accept error: {}", ec.message());
            continue;
        }

        // Disable Nagle for lower latency
        sock.set_option(tcp::no_delay(true));

        auto session = Session::create(
            std::move(sock), ssl_ctx_, cfg_, session_counter_);

        asio::co_spawn(
            ioc_,
            [s = std::move(session)]() mutable { return s->run(); },
            asio::detached);
    }
}

} // namespace jerboa
