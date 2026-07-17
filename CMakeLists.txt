cmake_minimum_required(VERSION 3.25)
project(ctypr LANGUAGES C)

# SQLite3 - Database (using same approach as C++ version)
include(FetchContent)
FetchContent_Declare(
    sqlite3
    URL https://sqlite.org/2026/sqlite-amalgamation-3530300.zip
)
# FetchContent_MakeAvailable(sqlite3)

FetchContent_GetProperties(sqlite3)
if(NOT sqlite3_POPULATED)
    FetchContent_Populate(sqlite3)
    
    # Create the static library target using the downloaded C file
    add_library(sqlite3 STATIC "${sqlite3_SOURCE_DIR}/sqlite3.c")
    
    # Make sure targets linking to this can find "sqlite3.h"
    target_include_directories(sqlite3 PUBLIC
        $<BUILD_INTERFACE:${sqlite3_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )
    
    # Link thread support (required by SQLite)
    find_package(Threads REQUIRED)
    target_link_libraries(sqlite3 PRIVATE Threads::Threads)
endif()

# Get version from git tags (e.g., v0.4.0 -> 0.4.0)
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(GIT_TAG MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    set(PROJECT_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
else()
    set(PROJECT_VERSION "0.1.0")  # Fallback if no tag
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Library ----

# Collect all source files
file(GLOB_RECURSE CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/content/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/format/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/db/*.c
)

# Configure version.h from template into build tree
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/core/version.h
)

# Copy public headers to build tree so generated version.h sits alongside them
file(GLOB CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.h
)
foreach(header ${CORE_HEADERS})
    configure_file(
        ${header}
        ${CMAKE_CURRENT_BINARY_DIR}/include/core/${CMAKE_FILE_NAME}
        COPYONLY
    )
endforeach()

# Create the static library
add_library(ctypr STATIC ${CORE_SOURCES})

# Set include directories — build tree has generated version.h + copied headers
target_include_directories(ctypr PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/content>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/format>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/db>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link SQLite
target_link_libraries(ctypr PRIVATE sqlite3)

# ---- Compile Options ----

if(MSVC)
    target_compile_options(ctypr PRIVATE /W3 /wd4267 /wd4244)
else()
    target_compile_options(ctypr PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- Tests ----

if(BUILD_TESTING OR NOT DEFINED BUILD_TESTING)
    enable_testing()
    
    # Add a custom target for running tests with output
    add_custom_target(run-tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS test_ctypr
        COMMENT "Running all tests with verbose output"
    )
    
    add_subdirectory(tests)
endif()

# ---- Installation ----

# Set install directories
set(CMAKE_INSTALL_INCLUDEDIR "include/ctypr")

# Install the library (include sqlite3 in export set, matching C++ approach)
install(TARGETS ctypr sqlite3
    EXPORT ctyprTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/core/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Generate and install CMake config files
include(CMakePackageConfigHelpers)

install(EXPORT ctyprTargets
    FILE ctyprTargets.cmake
    NAMESPACE ctypr::
    DESTINATION lib/cmake/ctypr
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ctyprConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ctyprConfig.cmake"
    INSTALL_DESTINATION lib/cmake/ctypr
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ctyprConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ctyprConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ctyprConfigVersion.cmake"
    DESTINATION lib/cmake/ctypr
)

# ---- Packaging (CPack) ----

set(CPACK_PACKAGE_NAME "ctypr")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C typing practice library")
set(CPACK_PACKAGE_VENDOR "ctypr")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_GENERATOR "TGZ;ZIP")

# Only set license if file exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

include(CPack)