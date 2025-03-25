#pragma once

#include <string.h> // strcmp
#include <errno.h>
#ifdef _GLIBCXX_HAVE_DIRENT_H
#ifdef _GLIBCXX_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <dirent.h> // opendir, readdir, fdopendir, dirfd
#ifdef _GLIBCXX_HAVE_FCNTL_H
#include <fcntl.h>  // open, openat, fcntl, AT_FDCWD, O_NOFOLLOW etc.
#include <unistd.h> // close, unlinkat
#endif
#endif

namespace std _GLIBCXX_VISIBILITY(default) {
    _GLIBCXX_BEGIN_NAMESPACE_VERSION
    namespace filesystem {
        namespace __gnu_posix {
            using char_type = char;
            using DIR       = ::DIR;
            typedef struct ::dirent dirent;
            using ::closedir;
            using ::opendir;
            using ::readdir;
        } // namespace __gnu_posix

        namespace posix = __gnu_posix;

        inline bool is_permission_denied_error(int e) { return (e == EACCES); }

        struct _Dir_base {
            // If no error occurs then dirp is non-null,
            // otherwise null (even if a permission denied error is ignored).
            _Dir_base(int fd, const posix::char_type* pathname, bool skip_permission_denied, bool nofollow, error_code& ec) noexcept
                : dirp(_Dir_base::openat(fd, pathname, nofollow))
            {
                if (dirp)
                    ec.clear();
                else if (is_permission_denied_error(errno) && skip_permission_denied)
                    ec.clear();
                else
                    ec.assign(errno, std::generic_category());
            }

            _Dir_base(_Dir_base&& d)
                : dirp(std::exchange(d.dirp, nullptr))
            {
            }

            _Dir_base& operator=(_Dir_base&&) = delete;

            ~_Dir_base()
            {
                if (dirp) posix::closedir(dirp);
            }

            const posix::dirent* advance(bool skip_permission_denied, error_code& ec) noexcept
            {
                ec.clear();

                int                  err  = std::exchange(errno, 0);
                const posix::dirent* entp = posix::readdir(dirp);
                err                       = std::exchange(errno, err);

                if (entp) {
                    // skip past dot and dot-dot
                    if (is_dot_or_dotdot(entp->d_name)) return advance(skip_permission_denied, ec);
                    return entp;
                } else if (err) {
                    if (err == EACCES && skip_permission_denied) return nullptr;
                    ec.assign(err, std::generic_category());
                    return nullptr;
                } else {
                    // reached the end
                    return nullptr;
                }
            }

            static constexpr int fdcwd() noexcept
            {
                return -1; // Use invalid fd if AT_FDCWD isn't supported.
            }

            static bool is_dot_or_dotdot(const char* s) noexcept { return !strcmp(s, ".") || !strcmp(s, ".."); }

            static posix::DIR* openat(int, const posix::char_type* pathname, bool) { return posix::opendir(pathname); }

            posix::DIR* dirp;
        };

    } // namespace filesystem

    // BEGIN/END macros must be defined before including this file.
    _GLIBCXX_BEGIN_NAMESPACE_FILESYSTEM

    inline file_type get_file_type(const std::filesystem::__gnu_posix::dirent& d [[gnu::unused]])
    {
#ifdef _DIRENT_HAVE_D_TYPE
        switch (d.d_type) {
        case DT_BLK:
            return file_type::block;
        case DT_CHR:
            return file_type::character;
        case DT_DIR:
            return file_type::directory;
        case DT_FIFO:
            return file_type::fifo;
        case DT_LNK:
            return file_type::symlink;
        case DT_REG:
            return file_type::regular;
        case DT_SOCK:
            return file_type::socket;
        case DT_UNKNOWN:
            return file_type::unknown;
        default:
            return file_type::none;
        }
#else
        return file_type::none;
#endif
    }

    _GLIBCXX_END_NAMESPACE_FILESYSTEM

    _GLIBCXX_END_NAMESPACE_VERSION
} // namespace std _GLIBCXX_VISIBILITY(default)
