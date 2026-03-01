# 编译 O0 版本
add_executable(linux_O0 test.c)
target_compile_options(linux_O0 PRIVATE -O0 -no-pie)
# 关键：这里定义宏 O0
target_compile_definitions(linux_O0 PRIVATE O0) 

# 编译 O2 版本
add_executable(linux_O2 test.c)
target_compile_options(linux_O2 PRIVATE -O2 -no-pie)
# 关键：这里定义宏 O2
target_compile_definitions(linux_O2 PRIVATE O2)