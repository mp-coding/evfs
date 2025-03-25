#pragma once

#include <memory>

namespace vfs {

    template <typename ValueT, typename MutexT> class locker;

    template <typename ValueT, typename MutexT> class locked {
    public:
        locked(const locked& oth)        = delete;
        locked& operator=(const locked&) = delete;
        locked& operator=(locked&&)      = delete;
        locked(locked&& oth)             = delete;

        ~locked() { mutex.unlock(); }
        ValueT& get() const { return value; }
        ValueT& operator*() const { return get(); }

    private:
        friend class locker<ValueT, MutexT>;

        locked(ValueT& value, MutexT& mutex)
            : value {value}
            , mutex {mutex}
        {
            mutex.lock();
        }
        ValueT& value;
        MutexT& mutex;
    };

    template <typename ValueT, typename MutexT> class locker {
    public:
        explicit locker(ValueT value)
            : value {std::move(value)}
        {
        }

        locked<ValueT, MutexT> lock() { return {value, mutex}; }

    private:
        ValueT value;
        MutexT mutex;
    };
} // namespace vfs
