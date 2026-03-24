#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  jerboa-tcp-tls :: session.hpp
//  One forwarding session:  plain_socket (client) ↔ TLS stream (remote).
// ─────────────────────────────────────────────────────────────────────────────

#include "config.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <atomic>
#include <memory>
#include <span>

namespace jerboa {

namespace asio = boost::asio;
namespace ssl  = boost::asio::ssl;
using tcp      = asio::ip::tcp;

// ── TLS stream type alias ─────────────────────────────────────────────────────
using TlsStream = ssl::stream<tcp::socket>;

// ─────────────────────────────────────────────────────────────────────────────
class Session : public std::enable_shared_from_this<Session> {
public:
    // Factory — always construct via make_shared
    [[nodiscard]] static std::shared_ptr<Session>
    create(tcp::socket       client_sock,
           ssl::context&     ssl_ctx,
           const Config&     cfg,
           std::atomic<uint64_t>& session_counter);

    // Start the async forwarding pipeline (non-blocking, co_spawn compatible)
    asio::awaitable<void> run();

private:
    Session(tcp::socket       client_sock,
            ssl::context&     ssl_ctx,
            const Config&     cfg,
            uint64_t          id);

    // ── Internal coroutines ───────────────────────────────────────────────────
    asio::awaitable<void> connect_remote();
    asio::awaitable<void> pump(tcp::socket& src, TlsStream& dst,
                               std::string_view direction);
    asio::awaitable<void> pump_reverse(TlsStream& src, tcp::socket& dst,
                                       std::string_view direction);

    // ── Data members ──────────────────────────────────────────────────────────
    tcp::socket   client_sock_;
    ssl::context& ssl_ctx_;
    const Config& cfg_;
    uint64_t      id_;            // session ID for log correlation

    // Outbound TLS stream (created lazily during connect_remote)
    std::unique_ptr<TlsStream> tls_stream_;

    static constexpr std::size_t kBufSize = 65'536; // 64 KiB
};

} // namespace jerboa
