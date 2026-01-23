# cpu/CMakeLists.txt

# 定义第一个 CPU 专项程序：用户态高占用
add_executable(cpu_high_user cpu_high_user.cpp)
add_executable(cpu_high_sys cpu_high_sys.cpp)
add_executable(cpu_context_switch cpu_context_switch.cpp) # 新增
add_executable(cpu_false_sharing_good cpu_false_sharing_good.cpp)
add_executable(cpu_false_sharing_bad cpu_false_sharing_bad.cpp)