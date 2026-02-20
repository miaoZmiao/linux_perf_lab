
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(mem_leak mem_leak.cpp)
add_executable(mem_seq_vs_rand mem_seq_vs_rand.cpp)

# 链接你刚刚构建的动态库
target_link_libraries(mem_seq_vs_rand perf_client)

# 定义脚本的文件名
set(SCRIPT_NAMES "mem_seq_vs_rand.sh" "perf_auto_record.sh")

foreach(SCRIPT ${SCRIPT_NAMES})
    # 获取源文件路径
    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}")
    # 获取构建目录下的目标路径
    set(DST "${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT}")

    # 每次编译（make）时，如果脚本有更新，自动复制
    add_custom_command(
        OUTPUT ${DST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DST}
        DEPENDS ${SRC}
        COMMENT "Copying ${SCRIPT} to build directory"
    )
    
    # 将拷贝动作关联到主构建流程中
    list(APPEND SCRIPT_TARGETS ${DST})
endforeach()

# 创建一个自定义 target 确保这些文件被处理
add_custom_target(mem_scripts_sync ALL DEPENDS ${SCRIPT_TARGETS})

