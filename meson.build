project(
    'PostgresCxxClient',
    'cpp',
    version: '1.0.1',
    license: 'MIT',
    meson_version: '>=1.2.0',
    default_options: {
        'buildtype': 'release',
        'optimization': '3',
        'cpp_std': 'c++17',
        'warning_level': '3',
        'werror': false,
        'b_ndebug': 'if-release',
    },
)

deps = []
src_files = []
inc_dirs = include_directories('include')

postgres_version_range = get_option('postgres_version_range')

if postgres_version_range == ''
    deps += dependency('libpq', 'PostgreSQL')
else

    if postgres_version_range.contains(',')
        postgres_version_range = postgres_version_range.split(',')
    endif

    deps += dependency(
        'libpq',
        'PostgreSQL',
        version: postgres_version_range,
    )
endif

subdir('src')

client_lib = library(
    'postgres-cxx-client',
    src_files,
    dependencies: deps,
    include_directories: inc_dirs,
)

client_dep = declare_dependency(
    include_directories: inc_dirs,
    link_with: client_lib,
    dependencies: deps,
    version: meson.project_version(),
)

if get_option('examples')
    subdir('examples')
endif

if get_option('tests')
    subdir('tests/unit')
endif
