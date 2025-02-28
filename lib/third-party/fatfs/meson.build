_inc = include_directories('source')

_src = files (
    'source/ff.c',
    'source/ffunicode.c',
)

_deps = [fatfs_config_dep]

c_opt_args = ['-Wno-overflow']

libfatfs = library('fatfs', sources : _src, include_directories : _inc, dependencies : _deps, c_args : c_opt_args)
libfatfs_dep = declare_dependency(link_with : libfatfs, include_directories : _inc, dependencies : _deps)