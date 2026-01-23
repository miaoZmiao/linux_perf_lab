# gaussdb/CMakeLists.txt
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
# 添加可执行文件
add_executable(gaussdb gaussdb1.cpp)

# 确保目标名称与内核模块中追踪的名称一致 (这是您的需求)
set_target_properties(gaussdb PROPERTIES OUTPUT_NAME "gaussdb")

