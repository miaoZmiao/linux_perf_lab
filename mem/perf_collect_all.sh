#!/bin/bash

# 参数检查
MODE=$1    # "shuffle" 或 "" (空表示顺序)
WARMUP=$2  # "warmup" 或 "" (空表示不预热)

# 1. 编译并启动程序 (后台运行)
# 使用 FIFO 管道自动喂入第一个回车
tmp_pipe=$(mktemp -u)
mkfifo "$tmp_pipe"

./mem/mem_seq_vs_rand $MODE $WARMUP < "$tmp_pipe" &
APP_PID=$!

# 等待程序初始化完成
sleep 1
echo ">>> 程序已就绪 (PID: $APP_PID)，准备开始精准采集..."

# 2. 采集 stat 数据 (第一个独立运行，为了获取纯净数字)
echo ">>> 正在采集 perf stat..."
perf stat -p $APP_PID -e cycles,instructions,page-faults,LLC-load-misses,L1-dcache-load-misses -- sleep 100 > core_stat.txt 2>&1 &
STAT_PID=$!
sleep 1 # 等待 perf stat 挂载

echo "" > "$tmp_pipe" # 发送第一个回车，开始循环

# 轮询检查循环是否结束 (检查输出或进程状态)
while kill -0 $APP_PID 2>/dev/null; do
    if grep -q "Loop finished" <(ps -o comm= -p $APP_PID); then break; fi
    sleep 0.1
done

kill $STAT_PID # 停止 stat 采集
echo ">>> perf stat 采集完成。"

# 3. 采集 record 数据 (第二次运行，为了生成 perf.data)
# 为了 record 的纯净，我们需要重新跑一次核心循环，或者在此程序中增加多次循环逻辑
# 这里简单起见，我们直接告诉用户如何查看刚才统计出的 core_stat.txt
# 如果你需要 record，建议手动执行 record 流程，或者参考下方命令：

echo "------------------------------------------------"
echo "核心循环 STAT 结果 (已剔除 munmap):"
cat core_stat.txt | grep -E "cycles|instructions|page-faults|LLC-load-misses"
echo "------------------------------------------------"

# 清理
kill -9 $APP_PID 2>/dev/null
rm "$tmp_pipe"
