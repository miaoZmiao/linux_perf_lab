# --- 案例 A: 内联汇编立即数约束 (test.c) ---

# O2 成功版
add_executable(linux_O2 test.c)
target_compile_options(linux_O2 PRIVATE -O2 -no-pie)
target_compile_definitions(linux_O2 PRIVATE O2)

# O0 失败版 (EXCLUDE_FROM_ALL: 默认不编译)
add_executable(linux_O0 EXCLUDE_FROM_ALL test.c)
target_compile_options(linux_O0 PRIVATE -O0 -no-pie)
target_compile_definitions(linux_O0 PRIVATE O0)


# --- 案例 B: 符号引用与死代码消除 (main.c) ---

# O2 成功版
add_executable(app_o2 main.c)
target_compile_options(app_o2 PRIVATE -O2 -no-pie)

# O0 失败版 (EXCLUDE_FROM_ALL: 默认不编译)
add_executable(app_o0 EXCLUDE_FROM_ALL main.c)
target_compile_options(app_o0 PRIVATE -O0 -no-pie)
set_target_properties(app_o0 PROPERTIES OUTPUT_NAME "fail_with_O0")

# ----------------------------------------------------------------
# 提示信息
# ----------------------------------------------------------------
message(STATUS "配置完成。")
message(STATUS "提示: 默认构建仅包含 O2 成功版本。")
message(STATUS "若要复现 O0 失败现象，请手动执行:")
message(STATUS "  make linux_O0  (测试内联汇编约束失败)")
message(STATUS "  make app_o0    (测试链接符号缺失失败)")