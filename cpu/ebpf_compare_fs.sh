#!/bin/bash
# 检查权限
if [ "$EUID" -ne 0 ]; then echo "请使用 sudo 运行"; exit 1; fi

run_test() {
    local name=$1
    local bin=$2
    
    echo "-------------------------------------------"
    echo "正在测试: $name"
    
    # 1. 后台启动 eBPF 采样器 (全系统采样)
    # 增加 -F 49 降低采样频率，避免在虚拟机里产生过大负载
    profile-bpfcc -F 49 15 > "ebpf_${name}.txt" 2>/dev/null &
    EBPF_PID=$!
    
    echo "正在预热 eBPF (5秒)..."
    sleep 5
    
    echo "启动程序 $bin..."
    # 2. 使用 perf 记录。如果 hardware counters 不可用，perf 会自动回退
    # 我们强制记录 task-clock (毫秒)，这是虚拟机支持的软件事件
    sudo perf stat -e cpu-clock,faults,cs -o "perf_${name}.txt" $bin
    
    echo "等待 eBPF 完成采样..."
    wait $EBPF_PID
}

# 确保路径正确 (根据你的实际 build 路径调整)
run_test "bad" "../build/cpu/cpu_false_sharing_bad"
run_test "good" "../build/cpu/cpu_false_sharing_good"

echo "==========================================="
echo "对比分析结果:"
# 提取 task-clock (CPU 占用时间) 进行对比
BAD_TIME=$(grep "cpu-clock" perf_bad.txt | awk '{print $1}' | sed 's/,//g')
GOOD_TIME=$(grep "cpu-clock" perf_good.txt | awk '{print $1}' | sed 's/,//g')

echo "BAD  版本消耗 CPU 时间: ${BAD_TIME} msec"
echo "GOOD 版本消耗 CPU 时间: ${GOOD_TIME} msec"

# 计算倍数
if [ $(echo "$GOOD_TIME > 0" | bc) -ne 0 ]; then
    RATIO=$(echo "scale=2; $BAD_TIME / $GOOD_TIME" | bc)
    echo "性能差距: Bad 版本比 Good 版本慢了 $RATIO 倍"
fi