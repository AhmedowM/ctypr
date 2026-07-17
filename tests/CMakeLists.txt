# Test executable
add_executable(test_ctypr
    test_engine.c
)

target_link_libraries(test_ctypr PRIVATE ctypr)

# Register tests
add_test(NAME test_engine COMMAND test_ctypr)