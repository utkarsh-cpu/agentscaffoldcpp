cmake_minimum_required(VERSION 3.16)

# Find Google Test
find_package(GTest REQUIRED)

# Include Google Test
include(GoogleTest)

# Function to create test executables
function(add_pocketflow_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE 
        pocketflow_cpp 
        nlohmann_json::nlohmann_json
        GTest::gtest 
        GTest::gtest_main
    )
    target_compile_features(${name} PRIVATE cxx_std_17)
    
    # Discover tests
    gtest_discover_tests(${name})
endfunction()

# Add test executables
add_pocketflow_test(test_base_node)
add_pocketflow_test(test_node)
add_pocketflow_test(test_batch_node)
add_pocketflow_test(test_flow)
add_pocketflow_test(test_batch_flow)
add_pocketflow_test(test_async_node)
add_pocketflow_test(test_async_batch_node)
add_pocketflow_test(test_async_flow)
add_pocketflow_test(test_integration)
add_pocketflow_test(test_performance_benchmark)
add_pocketflow_test(test_end_to_end)
add_pocketflow_test(test_async_debug)
add_pocketflow_test(test_api_compatibility)
add_pocketflow_test(test_memory_benchmark)

# Enable testing
enable_testing()