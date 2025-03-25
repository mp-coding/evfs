#include "vfs/logger.hpp"

namespace vfs::logger {
    output_handler_t g_write_output{};

    void register_output_callback(output_handler_t cb) noexcept { g_write_output = std::move(cb); }

    namespace internal {
        void write_output(const level lvl, const char* buf) noexcept
        {
            if (g_write_output) { g_write_output(lvl, buf); }
        }
    } // namespace internal
} // namespace vfs::logger