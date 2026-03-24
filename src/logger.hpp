#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  jerboa-tcp-tls :: logger.hpp
//  Thread-safe, timestamped console logger using std::print (C++23).
// ─────────────────────────────────────────────────────────────────────────────

#include <chrono>
#include <format>
#include <mutex>
#include <print>
#include <string_view>

namespace jerboa {

enum class LogLevel : int {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
};

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void set_level(LogLevel level) noexcept { min_level_ = level; }

    template<typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (level < min_level_) return;
        const auto msg = std::format(fmt, std::forward<Args>(args)...);
        const auto ts  = timestamp();
        const auto lbl = label(level);

        std::lock_guard lock{mu_};
        std::println("{} {} {}", ts, lbl, msg);
    }

private:
    Logger() = default;

    static std::string timestamp() {
        using namespace std::chrono;
        const auto now  = system_clock::now();
        const auto time = system_clock::to_time_t(now);
        const auto ms   = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::tm tm{};
        gmtime_r(&time, &tm);
        return std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:03d}Z",
                           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                           tm.tm_hour, tm.tm_min, tm.tm_sec,
                           static_cast<int>(ms.count()));
    }

    static constexpr std::string_view label(LogLevel l) noexcept {
        switch (l) {
            case LogLevel::Debug: return "[DEBUG]";
            case LogLevel::Info:  return "[INFO] ";
            case LogLevel::Warn:  return "[WARN] ";
            case LogLevel::Error: return "[ERROR]";
        }
        return "[?????]";
    }

    std::mutex mu_;
    LogLevel   min_level_{LogLevel::Info};
};

// ── Convenience macros ────────────────────────────────────────────────────────
#define LOG_DEBUG(...) ::jerboa::Logger::instance().log(::jerboa::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)  ::jerboa::Logger::instance().log(::jerboa::LogLevel::Info,  __VA_ARGS__)
#define LOG_WARN(...)  ::jerboa::Logger::instance().log(::jerboa::LogLevel::Warn,  __VA_ARGS__)
#define LOG_ERROR(...) ::jerboa::Logger::instance().log(::jerboa::LogLevel::Error, __VA_ARGS__)

} // namespace jerboa
