#!/bin/bash
# 伪共享 eBPF + Perf 深度定量对比脚本

if [ "$EUID" -ne 0 ]; then echo "请使用 sudo 运行"; exit 1; fi

# 定义测试函数
run_analysis() {
    local name=$1
    local bin=$2
    
    echo "==========================================="
    echo "正在测试 [$name] 版本..."
    
    # 1. 后台启动 stackcount 统计跨核中断函数
    # 采样 10 秒，并将结果输出到临时文件 sysvec_call_function_single smp_call_function_single
    stackcount-bpfcc -D 10 "smp_call_function_single" > "stack_${name}.txt" 2>/dev/null &
    STACK_PID=$!
    
    echo "等待 eBPF 预热并启动程序 (10秒采样开始)..."
    sleep 2
    
    # 2. 运行程序并记录 CPU 时间
    # 使用 sudo perf stat 记录 task-clock
    sudo perf stat -e task-clock -o "perf_${name}.txt" $bin
    
    # 等待 stackcount 结束
    wait $STACK_PID
    echo "[$name] 测试完成。"
}

# 确保二进制文件存在
if [ ! -f "../build/cpu/cpu_false_sharing_bad" ]; then echo "找不到 Bad 二进制文件"; exit 1; fi

# 执行两次测试
run_analysis "BAD" "../build/cpu/cpu_false_sharing_bad"
run_analysis "GOOD" "../build/cpu/cpu_false_sharing_good"

# --- 结果定量汇总 ---
echo ""
echo "###########################################"
echo "        伪共享定量分析报告"
echo "###########################################"

# 1. 时间维度
TIME_BAD=$(grep "task-clock" perf_BAD.txt | awk '{print $1}' | sed 's/,//g')
TIME_GOOD=$(grep "task-clock" perf_GOOD.txt | awk '{print $1}' | sed 's/,//g')
RATIO=$(echo "scale=2; $TIME_BAD / $TIME_GOOD" | bc)

# 2. 中断维度 (eBPF 采集到的次数)
# stackcount 的输出末尾通常是次数
COUNT_BAD=$(grep -A 1 "smp_call_function_single" stack_BAD.txt | tail -n 1 | awk '{print $1}')
COUNT_GOOD=$(grep -A 1 "smp_call_function_single" stack_GOOD.txt | tail -n 1 | awk '{print $1}')

# 如果为空则设为 0
COUNT_BAD=${COUNT_BAD:-0}
COUNT_GOOD=${COUNT_GOOD:-0}

echo "| 指标名称            | BAD 版本      | GOOD 版本     | 结论"
echo "|---------------------|---------------|---------------|----------------"
echo "| CPU时间 (ms)        | $TIME_BAD      | $TIME_GOOD      | 慢了 $RATIO 倍"
echo "| 跨核同步中断 (次数) | $COUNT_BAD           | $COUNT_GOOD           | 冲突程度定量"

echo ""
echo "分析提示："
echo "1. 如果 BAD 版本的中断次数远高于 GOOD，说明硬件缓存一致性协议在疯狂打架。"
echo "2. task-clock 的巨大差异定量反映了这些中断和总线锁定带来的性能损耗。"