cmake_minimum_required(VERSION 3.25)
project(ctypr LANGUAGES C)

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
    set(PROJECT_VERSION "0.4.0")  # Fallback if no tag
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Dependencies via FetchContent ----
include(FetchContent)

# SQLite3 - Database
# FetchContent_Declare(
#     sqlite3
#     URL https://sqlite.org/2026/sqlite-amalgamation-3530300.zip
# )
# # FetchContent_MakeAvailable(sqlite3)

# FetchContent_GetProperties(sqlite3)
# if(NOT sqlite3_POPULATED)
#     FetchContent_Populate(sqlite3)
    
#     # Create the static library target using the downloaded C file
#     add_library(sqlite3 STATIC "${sqlite3_SOURCE_DIR}/sqlite3.c")
    
#     # Make sure targets linking to this can find "sqlite3.h"
#     target_include_directories(sqlite3 PUBLIC "${sqlite3_SOURCE_DIR}")
    
#     # Link thread support (required by SQLite)
#     find_package(Threads REQUIRED)
#     target_link_libraries(sqlite3 PRIVATE Threads::Threads)
# endif()

# ---- Library ----

# ---- Compile Options ----

# target_compile_features(ctypr PRIVATE c_std_17)
# target_compile_definitions(ctypr PRIVATE
#     SQLITE_THREADSAFE=0
#     SQLITE_OMIT_LOAD_EXTENSION
# )

# Link FetchContent dependencies
# target_link_libraries(typr PRIVATE sqlite3)
# if(MSVC)
#     target_compile_options(typr PRIVATE /W3 /wd4267 /wd4244)
# endif()

# ---- Tests ----
# if(BUILD_TESTING)
#     enable_testing()
#     add_subdirectory(tests)
# endif()

# ---- Installation ----

# Set install directories
set(CMAKE_INSTALL_INCLUDEDIR "include")

# Install the library
# install(TARGETS ctypr
#     EXPORT typrTargets
#     ARCHIVE DESTINATION lib
#     LIBRARY DESTINATION lib
#     RUNTIME DESTINATION bin
# )

# Generate and install CMake config files
# include(CMakePackageConfigHelpers)

# install(EXPORT typrTargets
#     FILE typrTargets.cmake
#     NAMESPACE typr::
#     DESTINATION lib/cmake/typr
# )

# configure_package_config_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/typrConfig.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/typrConfig.cmake"
#     INSTALL_DESTINATION lib/cmake/typr
# )

# write_basic_package_version_file(
#     "${CMAKE_CURRENT_BINARY_DIR}/typrConfigVersion.cmake"
#     VERSION ${PROJECT_VERSION}
#     COMPATIBILITY SameMajorVersion
# )

# install(FILES
#     "${CMAKE_CURRENT_BINARY_DIR}/typrConfig.cmake"
#     "${CMAKE_CURRENT_BINARY_DIR}/typrConfigVersion.cmake"
#     DESTINATION lib/cmake/typr
# )

# ---- Packaging (CPack) ----
# set(CPACK_PACKAGE_NAME "typr")
# set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
# set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++ typing practice library")
# set(CPACK_PACKAGE_VENDOR "typr")
# set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
# set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
# set(CPACK_GENERATOR "TGZ;ZIP")

# Only set license if file exists
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
#     set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
# endif()

# include(CPack)