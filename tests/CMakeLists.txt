function(ctypr_add_test name source)
    add_executable(test_${name} ${source})
    target_link_libraries(test_${name} PRIVATE ctypr ${ARGN})
    add_test(NAME ${name} COMMAND test_${name})
    set_tests_properties(${name} PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endfunction()

ctypr_add_test(engine test_engine.c)
ctypr_add_test(content test_content.c sqlite3)
ctypr_add_test(formatter test_formatter.c)
ctypr_add_test(repository test_repository.c)
ctypr_add_test(logger test_logger.c)
ctypr_add_test(concurrency test_concurrency.c)
