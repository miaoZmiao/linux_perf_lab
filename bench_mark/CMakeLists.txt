# 确保使用当前 CPU 的最优指令集
# -O1 足够防止循环被完全优化掉，同时保持内联汇编的意图
set(CMAKE_CXX_FLAGS " -O1 -march=native")

# 添加可执行文件
add_executable(ipc_limit ipc_limit.cpp)

# 如果你的项目需要链接之前的 perf 库，可以加上：
# target_link_libraries(ipc_limit perf_client)

