# Engine tests
add_executable(test_engine
    test_engine.c
)
target_link_libraries(test_engine PRIVATE ctypr)
add_test(NAME engine COMMAND test_engine)
set_tests_properties(engine PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Content provider tests
add_executable(test_content
    test_content.c
)
target_link_libraries(test_content PRIVATE ctypr)
add_test(NAME content COMMAND test_content)
set_tests_properties(content PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
