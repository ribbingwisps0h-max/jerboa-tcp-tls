// ─────────────────────────────────────────────────────────────────────────────
//  jerboa-tcp-tls :: session.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include "session.hpp"
#include "logger.hpp"

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/system/system_error.hpp>

#include <array>
#include <span>
#include <string_view>
#include <system_error>

namespace jerboa {

using namespace boost::asio::experimental::awaitable_operators;

// ── Factory ───────────────────────────────────────────────────────────────────
std::shared_ptr<Session>
Session::create(tcp::socket           client_sock,
                ssl::context&         ssl_ctx,
                const Config&         cfg,
                std::atomic<uint64_t>& session_counter) {
    uint64_t id = ++session_counter;
    // Can't use make_shared because ctor is private; use direct new + shared_ptr
    return std::shared_ptr<Session>(
        new Session(std::move(client_sock), ssl_ctx, cfg, id));
}

Session::Session(tcp::socket   client_sock,
                 ssl::context& ssl_ctx,
                 const Config& cfg,
                 uint64_t      id)
    : client_sock_(std::move(client_sock))
    , ssl_ctx_(ssl_ctx)
    , cfg_(cfg)
    , id_(id) {}

// ── run ───────────────────────────────────────────────────────────────────────
asio::awaitable<void> Session::run() {
    try {
        co_await connect_remote();

        using namespace boost::asio::experimental::awaitable_operators;
        // Запускаем два потока данных параллельно
        co_await (pump(client_sock_, *tls_stream_, ">>>") &&
                  pump_reverse(*tls_stream_, client_sock_, "<<<"));

    } catch (const std::exception& e) {
        LOG_DEBUG("[session {}] closed: {}", id_, e.what());
    }
    LOG_INFO("[session {}] terminated", id_);
}

// ── connect_remote ────────────────────────────────────────────────────────────
asio::awaitable<void> Session::connect_remote() {
    auto executor = co_await asio::this_coro::executor;

    LOG_DEBUG("[session {}] resolving {}:{}", id_,
              cfg_.remote_host, cfg_.remote_port);

    tcp::resolver resolver{executor};
    auto endpoints = co_await resolver.async_resolve(
        cfg_.remote_host, cfg_.remote_port, asio::use_awaitable);

    tls_stream_ = std::make_unique<TlsStream>(executor, ssl_ctx_);

    // Set SNI hostname
    if (!SSL_set_tlsext_host_name(tls_stream_->native_handle(),
                                  cfg_.remote_host.c_str())) {
        throw std::runtime_error("SSL_set_tlsext_host_name failed");
    }

    co_await asio::async_connect(
        tls_stream_->lowest_layer(), endpoints, asio::use_awaitable);

    LOG_DEBUG("[session {}] TCP connected to {}:{}, starting TLS handshake",
              id_, cfg_.remote_host, cfg_.remote_port);

    co_await tls_stream_->async_handshake(
        ssl::stream_base::client, asio::use_awaitable);

    // Log cipher and protocol
    const char* cipher  = SSL_get_cipher(tls_stream_->native_handle());
    const char* version = SSL_get_version(tls_stream_->native_handle());
    LOG_INFO("[session {}] TLS handshake OK  protocol={} cipher={}",
             id_, version ? version : "?", cipher ? cipher : "?");
}

// ── pump: plain socket → TLS stream ──────────────────────────────────────────
asio::awaitable<void>
Session::pump(tcp::socket& src, TlsStream& dst,
              std::string_view direction) {
    std::array<std::byte, kBufSize> buf{};
    std::span<std::byte> view{buf};

    for (;;) {
        std::size_t n = co_await src.async_read_some(
            asio::buffer(view.data(), view.size()), asio::use_awaitable);

        if (n == 0) break;

        co_await asio::async_write(
            dst, asio::buffer(view.data(), n), asio::use_awaitable);

        LOG_DEBUG("[session {}] {} {} bytes", id_, direction, n);
    }
}

// ── pump_reverse: TLS stream → plain socket ───────────────────────────────────
asio::awaitable<void>
Session::pump_reverse(TlsStream& src, tcp::socket& dst,
                      std::string_view direction) {
    std::array<std::byte, kBufSize> buf{};
    std::span<std::byte> view{buf};

    for (;;) {
        std::size_t n = co_await src.async_read_some(
            asio::buffer(view.data(), view.size()), asio::use_awaitable);

        if (n == 0) break;

        co_await asio::async_write(
            dst, asio::buffer(view.data(), n), asio::use_awaitable);

        LOG_DEBUG("[session {}] {} {} bytes", id_, direction, n);
    }
}

} // namespace jerboa
