// ─────────────────────────────────────────────────────────────────────────────
//  jerboa-tcp-tls :: main.cpp
//  Entry point: CLI argument parsing + server bootstrap.
// ─────────────────────────────────────────────────────────────────────────────

#include "config.hpp"
#include "logger.hpp"
#include "tls_server.hpp"

#include <cstdlib>
#include <expected>
#include <print>
#include <string>
#include <string_view>

// POSIX getopt_long
#include <getopt.h>

namespace {

// ── Banner ────────────────────────────────────────────────────────────────────
void print_banner() {
    std::println(R"(
  _
 (_) ___ _ __ | |__   ___   __ _       | |_ ___ _ __       | |_ | |___
 | |/ _ \ '__|| '_ \ / _ \ / _` |  _   | __/ __| '_ \  _   | __|| / __|
 | |  __/ |  || |_) | (_) | (_| | | |_  | || (__| |_) || |_  | |_ | \__ \
 |_|\___|_|  |_|___/ \___/ \__,_|  \__|  \__\___| .__/  \__|  \__||_|___/
                                                  |_|
  High-performance TLS 1.3 TCP tunnel forwarder — C++23 + Asio + OpenSSL 3.x
)");
}

// ── Usage ─────────────────────────────────────────────────────────────────────
void print_usage(std::string_view prog) {
    std::println(R"(
Usage: {} [OPTIONS]

Options:
  -l, --listen <port>       Local port to listen on                [required]
  -r, --remote <host:port>  Remote TLS target host:port            [required]
  -c, --cert   <file>       Client certificate PEM (for mTLS)
  -k, --key    <file>       Client private key PEM  (for mTLS)
  -a, --ca     <file>       CA certificate PEM for peer verification
  -m, --mtls                Enable mutual TLS (requires -c and -k)
      --no-verify           Disable remote certificate verification
  -d, --debug               Enable DEBUG log level
  -h, --help                Show this help and exit

Examples:
  # Forward local 8080 → example.com:443 over TLS
  {} -l 8080 -r example.com:443

  # With explicit CA bundle
  {} -l 8080 -r my-service:8443 -a /etc/ssl/certs/ca-certificates.crt

  # Mutual TLS
  {} -l 9000 -r service:8443 -a ca.pem -c client.crt -k client.key --mtls
)", prog, prog, prog, prog);
}

// ── parse_args ────────────────────────────────────────────────────────────────
std::expected<jerboa::Config, std::string>
parse_args(int argc, char* argv[]) {
    jerboa::Config cfg;
    bool no_verify = false;

    static const option long_opts[] = {
        {"listen",    required_argument, nullptr, 'l'},
        {"remote",    required_argument, nullptr, 'r'},
        {"cert",      required_argument, nullptr, 'c'},
        {"key",       required_argument, nullptr, 'k'},
        {"ca",        required_argument, nullptr, 'a'},
        {"mtls",      no_argument,       nullptr, 'm'},
        {"no-verify", no_argument,       nullptr, 'n'},  // 'n' internal only
        {"debug",     no_argument,       nullptr, 'd'},
        {"help",      no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    int opt_idx = 0;
    while ((opt = getopt_long(argc, argv, "l:r:c:k:a:mdh", long_opts, &opt_idx)) != -1) {
        switch (opt) {
        case 'l': {
            long port = std::strtol(optarg, nullptr, 10);
            if (port <= 0 || port > 65535) {
                return std::unexpected(
                    std::string("Invalid listen port: ") + optarg);
            }
            cfg.listen_port = static_cast<uint16_t>(port);
            break;
        }
        case 'r': {
            std::string remote{optarg};
            auto colon = remote.rfind(':');
            if (colon == std::string::npos) {
                return std::unexpected(
                    "Remote must be in host:port format, got: " + remote);
            }
            cfg.remote_host = remote.substr(0, colon);
            cfg.remote_port = remote.substr(colon + 1);
            break;
        }
        case 'c': cfg.cert_file  = optarg; break;
        case 'k': cfg.key_file   = optarg; break;
        case 'a': cfg.ca_file    = optarg; break;
        case 'm': cfg.mtls       = true;   break;
        case 'n': no_verify      = true;   break;
        case 'd': cfg.debug      = true;   break;
        case 'h':
            return std::unexpected(std::string{});  // empty = show help, no error
        default:
            return std::unexpected("Unknown option. Use -h for help.");
        }
    }

    if (no_verify) cfg.verify_peer = false;

    if (cfg.listen_port == 0) {
        return std::unexpected("--listen port is required");
    }
    if (cfg.remote_host.empty()) {
        return std::unexpected("--remote host:port is required");
    }

    return cfg;
}

} // anonymous namespace

// ── main ──────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    print_banner();

    auto cfg_result = parse_args(argc, argv);
    if (!cfg_result) {
        if (!cfg_result.error().empty()) {
            std::println(stderr, "Error: {}", cfg_result.error());
        }
        print_usage(argv[0]);
        return cfg_result.error().empty() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    const auto& cfg = *cfg_result;

    // Configure log level
    if (cfg.debug) {
        jerboa::Logger::instance().set_level(jerboa::LogLevel::Debug);
        LOG_DEBUG("Debug logging enabled");
    }

    LOG_INFO("jerboa-tcp-tls starting up");

    jerboa::TlsServer server(cfg);

    auto init_result = server.init();
    if (!init_result) {
        LOG_ERROR("Initialization failed: {}", init_result.error());
        return EXIT_FAILURE;
    }

    server.run();
    return EXIT_SUCCESS;
}
