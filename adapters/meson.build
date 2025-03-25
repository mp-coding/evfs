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

libvfs_syscalls = library('vfs_syscalls',
                          sources : _src,
                          include_directories : _inc,
                          dependencies : libvfs_dep,
)

libvfs_syscalls_dep = declare_dependency(
    link_with : libvfs_syscalls,
    include_directories : _inc_pub)


libvfs_syscalls_newlib = library('vfs_syscalls_newlib', sources : ['newlib/syscalls.cpp'],
                                 dependencies : [libvfs_syscalls_dep])

libvfs_syscalls_newlib_dep = declare_dependency(link_whole : libvfs_syscalls_newlib)



