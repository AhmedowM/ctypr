# Test executable
add_executable(test_ctypr
    test_engine.c
)

target_link_libraries(test_ctypr PRIVATE ctypr)

# Register test with CTest
add_test(NAME engine COMMAND test_ctypr)
set_tests_properties(engine PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
