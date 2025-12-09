

test_src_files = files(
    'src/ChannelFake.cpp',
    'src/ChannelMock.cpp',
    'src/ChannelTest.cpp',
    'src/ClientTest.cpp',
    'src/CommandTest.cpp',
    'src/ConfigTest.cpp',
    'src/ConnectionTest.cpp',
    'src/ContextTest.cpp',
    'src/DispatcherTest.cpp',
    'src/FieldTest.cpp',
    'src/PreparedCommandTest.cpp',
    'src/ReceiverTest.cpp',
    'src/ResultTest.cpp',
    'src/RowTest.cpp',
    'src/Samples.cpp',
    'src/StatementTest.cpp',
    'src/TableTest.cpp',
    'src/TimeTest.cpp',
    'src/TransactionTest.cpp',
    'src/WorkerTest.cpp',
    'src/main.cpp',
)

test_deps = [client_dep]

test_deps += dependency('gtest')
test_deps += dependency('gmock')

all_tests = executable(
    'PostgresCxxClientTest',
    test_src_files,
    dependencies: test_deps,
)


test('PostgresCxxClientTest', all_tests)
