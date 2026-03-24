#pragma once
#include <chrono>
#include <format>
#include <mutex>
#include <string_view>
#include <iostream>

#if __has_include(<print>)
#include <print>
#define HAS_PRINT 1
#else
#define HAS_PRINT 0
#endif

namespace jerboa {
    enum class LogLevel : int { Debug = 0, Info = 1, Warn = 2, Error = 3 };

    class Logger {
    public:
        static Logger& instance() { static Logger inst; return inst; }
        void set_level(LogLevel level) noexcept { min_level_ = level; }

        template<typename... Args>
        void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
            if (level < min_level_) return;
            const auto msg = std::format(fmt, std::forward<Args>(args)...);
            const auto ts  = timestamp();
            const auto lbl = label(level);

            std::lock_guard lock{mu_};
#if HAS_PRINT
            std::println("{} {} {}", ts, lbl, msg);
#else
            std::cout << std::format("{} {} {}\n", ts, lbl, msg) << std::flush;
#endif
        }
    private:
        Logger() = default;
        static std::string timestamp() {
            auto now = std::chrono::system_clock::now();
            return std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::milliseconds>(now));
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
        LogLevel min_level_{LogLevel::Info};
    };
}
#define LOG_DEBUG(...) ::jerboa::Logger::instance().log(::jerboa::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)  ::jerboa::Logger::instance().log(::jerboa::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...)  ::jerboa::Logger::instance().log(::jerboa::LogLevel::Warn, __VA_ARGS__)
#define LOG_ERROR(...) ::jerboa::Logger::instance().log(::jerboa::LogLevel::Error, __VA_ARGS__)