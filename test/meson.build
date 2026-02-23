
test_headers = [include_directories('../src')
                , configuration_inc         # to use config
                ]

process_test = executable('process_test'
    , 'process_test.cpp'
    , '../src/Process.cpp'
    , '../src/NameValue.cpp'
    , dependencies: deps
    , include_directories : test_headers)
test('process_test', process_test)

# used to create logo
svg_test = executable('svg_test'
    , 'SvgTest.cpp'
    , dependencies: deps
    , include_directories : test_headers)
