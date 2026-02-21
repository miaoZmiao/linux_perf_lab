cmake_minimum_required(VERSION 3.10)
project(LinuxPerfLab)

set(CMAKE_CXX_STANDARD 11)

# -O0 确保编译器不优化掉我们的“模拟故障”代码
# -pthread 之后可能用到多线程竞争
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2  -fno-omit-frame-pointer -pthread")

add_subdirectory(perf)

add_subdirectory(cpu)
add_subdirectory(mem)
add_subdirectory(gaussdb)
add_subdirectory(bench_mark)
