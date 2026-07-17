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

# Formatter tests
add_executable(test_formatter
    test_formatter.c
)
target_link_libraries(test_formatter PRIVATE ctypr)
add_test(NAME formatter COMMAND test_formatter)
set_tests_properties(formatter PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Repository tests
add_executable(test_repository
    test_repository.c
)
target_link_libraries(test_repository PRIVATE ctypr)
add_test(NAME repository COMMAND test_repository)
set_tests_properties(repository PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
