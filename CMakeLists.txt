cmake_minimum_required(VERSION 3.5)     # CMake install : https://cmake.org/download/
project(h2downloads)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify MSVC UTF-8 encoding   
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

set(CURL_DIR "C:/vcpkg/installed/x64-windows/share/curl")
set(ZLIB_ROOT "C:/vcpkg/installed/x64-windows")
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" )

find_package(CURL CONFIG REQUIRED)

# aux_source_directory(./src srcs)

# add_executable(${PROJECT_NAME}
#     # WIN32 # If you need a terminal for debug, please comment this statement 
#     ${srcs} 
# ) 

# 单个连接多个任务并行下载 http2 protocol
# add_executable(h2-test
#     src/h2-test.cpp
# ) 
# target_link_libraries(h2-test PRIVATE 
#     CURL::libcurl
# )

# 单个连接多个任务并行下载 http2 protocol
add_executable(h2-multi-task
    src/h2-multi-task.cpp
) 
target_link_libraries(h2-multi-task PRIVATE 
    CURL::libcurl
)

# 多个线程多个链接连多个任务并行下载 http2 protocol
add_executable(h2-multi-thread
    src/h2-multi-thread.cpp
) 
target_link_libraries(h2-multi-thread PRIVATE 
    CURL::libcurl
)
