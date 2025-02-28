#pragma once

#include <functional>
#include <cstdio>

namespace vfs::logger {
    enum class level {
        debug,
        info,
        warning,
        error,
    };

    /** Callback used for output data */
    using output_handler_t = std::function<void(level, const char*)>;

    /**
     * @brief Logger register output function callback
     *
     * @param cb Logger function callback
     */
    void register_output_callback(output_handler_t cb) noexcept;

    namespace internal {
        constexpr const char* level2str(level lvl) noexcept
        {
            switch (lvl) {
            case level::debug:
                return "DEBUG";
            case level::info:
                return "INFO";
            case level::warning:
                return "WARN";
            case level::error:
                return "ERROR";
            }
            return "";
        }

        void write_output(level lvl, const char*) noexcept;
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
        template <typename... Args> void _log(level lvl, const Args&... args)
        {
            char buffer[128] {};
            snprintf(buffer, sizeof(buffer), args...);
            write_output(lvl, buffer);
        }
#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
    } // namespace internal
} // namespace vfs::logger
