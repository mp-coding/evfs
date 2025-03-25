project('evfs', 'cpp', 'c',
        version : '1.0.0',
        default_options : ['warning_level=2',
                           'c_std=c2x', 'build.c_std=c2x',
                           'cpp_std=c++23', 'build.cpp_std=c++23',
                           'default_library=static',
                           'b_pch=false',
                           'b_staticpic=false',
                           'b_pie=false',
                           'b_lundef=false', 'build.b_lundef=false',
                           'werror=true',
                           'build.werror=true',
        ])


if build_machine.system() == 'darwin'
    add_project_link_arguments('-Wl,-dead_strip', language : ['c', 'cpp'])
else
    add_project_link_arguments('-Wl,--gc-sections', language : ['c', 'cpp'])
endif

subdir('lib')
subdir('adapters')
# Enable and build unit tests only if configured as standalone project
if not meson.is_subproject()
    subdir('test')
endif