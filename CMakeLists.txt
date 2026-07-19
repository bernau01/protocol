cmake_minimum_required(VERSION 3.22)
project(protocol_tester VERSION 0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt)   

add_subdirectory(dependencies/cargs) 
add_subdirectory(dependencies/quill) 
add_subdirectory(dependencies/yaml-cpp) 
add_subdirectory(Protocol) 

file(GLOB_RECURSE SRC_SOURCES
    "src/*.c"
    "src/*.cpp"
    "src/**/*.c"
    "src/**/*.cpp"
)

add_executable(${PROJECT_NAME} 
    ${SRC_SOURCES}
)

add_compile_definitions(${PROJECT_NAME} PRIVATE
    PROTOCOL_LOGGING_ENABLED
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    protocol
    cargs
    quill::quill
    yaml-cpp::yaml-cpp
    fmt::fmt
)