_inc = include_directories(['.'])
_inc_pub = ['api']

_src = [
    'syscalls.cpp',
]

# Provide custom system headers if built as standalone module(unit tests).
# Otherwise, rely on target system to provide required headers
if not meson.is_subproject()
    _inc = [_inc, 'sys']
    _inc_pub += ['sys']
else
    _src = [_src, 'fs_dir.cpp']
endif

libevfs_syscalls = library('evfs_syscalls',
                          sources : _src,
                          include_directories : _inc,
                          dependencies : evfs_dep,
)

evfs_syscalls_dep = declare_dependency(
    link_with : libevfs_syscalls,
    include_directories : _inc_pub)


libevfs_syscalls_newlib = library('evfs_syscalls_newlib', sources : ['newlib/syscalls.cpp'],
                                 dependencies : [evfs_syscalls_dep])

evfs_syscalls_newlib_dep = declare_dependency(link_whole : libevfs_syscalls_newlib)



