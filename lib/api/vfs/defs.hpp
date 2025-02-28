/*
 * defs.hpp
 * Created on: 02/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include <cstdint>
#include <bitset>
#include <mutex>
#include <pthread.h>
#include <cassert>
#include <system_error>
#include <expected>

namespace vfs {
    template <typename T> using result = std::expected<T, std::error_code>;

    class Mutex {
    public:
        Mutex()
        {
            pthread_mutexattr_t mta;
            pthread_mutexattr_init(&mta);
            pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
            [[maybe_unused]] const auto ret = pthread_mutex_init(&m_lock, &mta);
            assert(ret == 0);
        }
        ~Mutex() { pthread_mutex_destroy(&m_lock); }

        void lock() { pthread_mutex_lock(&m_lock); }
        void unlock() { pthread_mutex_unlock(&m_lock); }
        bool try_lock() { return pthread_mutex_trylock(&m_lock) == 0; }

    private:
        pthread_mutex_t m_lock {};
    };

    using auto_lock = std::lock_guard<Mutex>;

    struct MountFlags {
        enum {
            read_only = 0,
            remount   = 5,
        };
    };
    using Flags = std::bitset<32>;

    inline std::error_code from_errno(const int err) { return {err, std::generic_category()}; }
    inline auto            error(const int err) { return std::unexpected(from_errno(err)); }
    inline auto            error(const std::error_code err) { return std::unexpected(err); }

    /// Root base name used by auto-mount when underlying filesystem does not provide info about partition's name/label. In that case, root_base will be used to
    /// create a unique partition's mount point name.
    constexpr auto root_base = "/volume";
} // namespace vfs
