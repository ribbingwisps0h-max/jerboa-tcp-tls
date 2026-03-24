#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  jerboa-tcp-tls :: config.hpp
//  Plain data struct holding the parsed CLI configuration.
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <string>

namespace jerboa {

struct Config {
    // ── Listen side ───────────────────────────────────────────────────────────
    uint16_t listen_port{0};

    // ── Remote target ─────────────────────────────────────────────────────────
    std::string remote_host;
    std::string remote_port;

    // ── TLS material ──────────────────────────────────────────────────────────
    std::string ca_file;        // PEM CA bundle for peer verification
    std::string cert_file;      // Client certificate PEM (mTLS)
    std::string key_file;       // Client private key PEM  (mTLS)

    // ── Flags ─────────────────────────────────────────────────────────────────
    bool mtls{false};           // Enable mutual TLS
    bool verify_peer{true};     // Verify remote certificate
    bool debug{false};          // Enable DEBUG log level
};

} // namespace jerboa
