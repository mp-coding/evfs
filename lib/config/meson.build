# Adds lwext4 configuration dependency. lwext4 require some configuration parameters either in a form of header placed
# under 'generated/ext4_config.h' or raw precompiler define set(CONFIG_USE_DEFAULT_CFG must be set to 1)

inc = include_directories('.')

lwext4_config_dep = declare_dependency(
    include_directories: inc
    )