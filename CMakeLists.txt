cmake_minimum_required(VERSION 3.16)
project(PocketFlowCPP VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Find dependencies
find_package(nlohmann_json REQUIRED)

# Optional: Find Google Test for testing
find_package(GTest QUIET)

# Create interface library for header-only design
add_library(pocketflow_cpp INTERFACE)

# Set include directories
target_include_directories(pocketflow_cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(pocketflow_cpp INTERFACE nlohmann_json::nlohmann_json)

# Compiler features
target_compile_features(pocketflow_cpp INTERFACE cxx_std_17)

# Optional: Add examples subdirectory if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# Optional: Add tests subdirectory if it exists and GTest is found
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt" AND GTest_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation configuration
include(GNUInstallDirs)

install(TARGETS pocketflow_cpp
    EXPORT pocketflow_cpp-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT pocketflow_cpp-targets
    FILE pocketflow_cpp-targets.cmake
    NAMESPACE pocketflow_cpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pocketflow_cpp
)

# Create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pocketflow_cpp-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pocketflow_cpp-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pocketflow_cpp
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/pocketflow_cpp-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pocketflow_cpp-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/pocketflow_cpp-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pocketflow_cpp
)