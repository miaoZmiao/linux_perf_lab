# 1. 编译动态库
add_library(perf_client SHARED perf_client.cpp)
target_include_directories(perf_client PUBLIC ${CMAKE_SOURCE_DIR}/include)

# 2. 编译服务端进程
add_executable(perf_server perf_server.cpp)

# 3. 安装/同步：确保 server 也在 build 目录下方便调试
add_custom_command(TARGET perf_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:perf_server> ${CMAKE_BINARY_DIR}/perf_server
)