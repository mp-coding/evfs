#pragma once

#include "vfs/logger.hpp"

namespace vfs {
    template <typename... Args> void log_debug(Args&&... args) { logger::internal::_log(logger::level::debug, args...); }
    template <typename... Args> void log_info(Args&&... args) { logger::internal::_log(logger::level::info, args...); }
    template <typename... Args> void log_warning(Args&&... args) { logger::internal::_log(logger::level::warning, args...); }
    template <typename... Args> void log_error(Args&&... args) { logger::internal::_log(logger::level::error, args...); }
} // namespace vfs