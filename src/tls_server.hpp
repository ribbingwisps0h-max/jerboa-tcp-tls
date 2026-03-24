#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  jerboa-tcp-tls :: tls_server.hpp
//  Async acceptor loop + SSL context bootstrap.
// ─────────────────────────────────────────────────────────────────────────────

#include "config.hpp"
#include "session.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/awaitable.hpp>

#include <atomic>
#include <expected>
#include <string>

namespace jerboa {

namespace asio = boost::asio;
namespace ssl  = boost::asio::ssl;
using tcp      = asio::ip::tcp;

// ─────────────────────────────────────────────────────────────────────────────
class TlsServer {
public:
    explicit TlsServer(const Config& cfg);

    // Synchronously build & validate the SSL context.
    // Returns std::unexpected<string> on any configuration error.
    [[nodiscard]] std::expected<void, std::string> init();

    // Runs the acceptor loop until the io_context is stopped.
    void run();

private:
    asio::awaitable<void> accept_loop();

    const Config&          cfg_;
    asio::io_context       ioc_;
    ssl::context           ssl_ctx_;
    tcp::acceptor          acceptor_;
    std::atomic<uint64_t>  session_counter_{0};
};

} // namespace jerboa
