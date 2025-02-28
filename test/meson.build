test_common = library('test-common',
                      sources : ['common/ram_blkdev.cpp', 'common/FilesystemUndertest.cpp'],
                      include_directories : '',
                      dependencies : [libvfs_dep],
)

test_common_dep = declare_dependency(
    link_with : test_common,
    include_directories : 'common',
    dependencies : libvfs_dep
)

if meson.project_name() == 'vfs'
    catch2_with_main_dep = catch2.get_variable('catch2_with_main_dep')
    catch2_dep = catch2.get_variable('catch2_dep')
endif

dummy_test = executable('Dummy', 'dummy.cpp', dependencies : [test_common_dep, catch2_with_main_dep], include_directories : include_directories('../lib'))
test('Dummy', dummy_test)

fd_test = executable('FileDescriptors', 'fd_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
test('FileDescriptors', fd_test)
#
dir_test = executable('Directories', 'dir_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
test('Directories', dir_test)
#
#tools_test = executable('Tools', 'tools_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
#test('Tools', tools_test)
#
vfs_test = executable('VFS', 'vfs_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
test('VFS', vfs_test)
#
#mt_test = executable('Multithreading', 'multithreading_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
#test('Multithreading', mt_test)
#
#syscalls_test = executable('Syscalls', 'syscalls/syscalls_test.cpp', '../adapters/syscalls/syscalls.cpp',
#                           dependencies : [test_common_dep, catch2_with_main_dep],
#                           include_directories : ['syscalls', '../adapters/syscalls']
#
#)
#test('Syscalls', syscalls_test)
#
## Executables below are not registered within Meson test suite. They can only be used manually.
#load_image = executable('LoadImage', 'load_image.cpp', dependencies : [test_common_dep, catch2_dep])
#perf_test = executable('PerfTest', 'perf_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
#multiaccess_test = executable('MultiAccessFD', 'multiaccess_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
#
#subdir('execs')
#custom_target('Copy exec scripts',
#              input : ['execs/execute.sh', 'stress_test1.sh'],
#              output : ['scripts'],
#              command : ['cp', '-R', '@INPUT@', '@OUTDIR@'],
#              install : false,
#              build_by_default : true)