cmake_minimum_required(VERSION 3.10)
project(LinuxPerfLab)

set(CMAKE_CXX_STANDARD 11)

# -O0 确保编译器不优化掉我们的“模拟故障”代码
# -pthread 之后可能用到多线程竞争
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -pthread")

add_subdirectory(cpu)