project(
    'PostgresCxxClient',
    'cpp',
    version: '1.0.3',
    license: 'MIT',
    meson_version: '>=1.2.0',
    default_options: {
        'buildtype': 'release',
        'optimization': '3',
        'cpp_std': 'c++17',
        'warning_level': '3',
        'werror': true,
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

compile_args = []

cpp = meson.get_compiler('cpp')

compile_args += cpp.get_supported_arguments(
    '-Wassign-enum',
    '-Wenum-conversion',
    '-Wenum-switch',
    '-fstrict-enums',
    '-Wenum-identifier',
    '-Wenum-compare',
    '-Wenum-enum-conversion',
    '-Wenum-float-conversion',
    '-Wenum-too-large',
    '-Wenum-compare-switch',
    '-Wenum-compare-conditional',
    '-fno-strict-aliasing',
    '-Wpointer-arith',
    '-Wmissing-declarations',
    '-Wformat=2',
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wold-style-definition',
    '-Wundef',
    '-Wunused',
    '-Wcast-align',
    '-Wmissing-noreturn',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wlogical-op',
    '-Wignored-qualifiers',
    '-Wredundant-decls',
    '-Wimplicit',
    '-Wnonnull',
    '-Winit-self',
    '-Wmain',
    '-Wmissing-braces',
    '-Wsequence-point',
    '-Wreturn-type',
    '-Wtrigraphs',
    '-Warray-bounds',
    '-Wwrite-strings',
    '-Waddress',
    '-Wint-to-pointer-cast',
    '-Wpointer-to-int-cast',
    '-Wempty-body',
    '-Wwrite-strings',
    '-Wstrict-aliasing',
    '-Wno-sign-compare',
    '-Wno-cast-function-type',
    '-Wno-unused-parameter',
    '-Wno-missing-field-initializers',
    '-Wno-type-limits',
    '-Wshadow',
    '-Wfloat-conversion',
)

subdir('src')

client_lib = library(
    'postgres-cxx-client',
    src_files,
    dependencies: deps,
    cpp_args: compile_args,
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
