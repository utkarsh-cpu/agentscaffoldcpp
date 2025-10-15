cmake_minimum_required(VERSION 3.16)

# Find dependencies
find_package(nlohmann_json REQUIRED)

# Function to create example executables
function(add_pocketflow_example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE pocketflow_cpp nlohmann_json::nlohmann_json)
    target_compile_features(${name} PRIVATE cxx_std_17)
endfunction()

# Add example executables
add_pocketflow_example(basic_sequential)
add_pocketflow_example(agent_pattern)
add_pocketflow_example(async_processing)
add_pocketflow_example(batch_processing)