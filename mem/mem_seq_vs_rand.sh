#!/bin/bash

# 1. 启动程序并获取 PID
# 我们把程序放到后台，并将它的标准输出重定向
tmp_pipe=$(mktemp -u)
mkfifo "$tmp_pipe"

./mem/mem_seq_vs_rand $1 $2 < "$tmp_pipe" &
APP_PID=$!

# 等待程序打印 PID 并准备就绪
sleep 1

echo ">>> 正在开启精准统计..."

# 2. 启动 perf stat，记录核心指标
# 注意：我们这里不带命令，直接 attach PID
perf stat -p $APP_PID -e cycles,instructions,page-faults,LLC-load-misses,L1-dcache-load-misses -- sleep 100 > core_perf.txt 2>&1 &
PERF_PID=$!

sleep 1 # 等待 perf 挂载完成

# 3. 触发核心循环
echo "" > "$tmp_pipe"

# 4. 监听程序输出，当看到 "Loop finished" 时，立刻杀掉 perf
# 这样 perf 记录的结果里就不会包含最后的 munmap（那 99% 的内核开销）
while true; do
    if grep -q "Loop finished" <(ps -o comm= -p $APP_PID); then
        # 这里我们简单点，等程序提示循环结束
        # 实际操作中，我们要手动在另一个终端点一下回车，或者用下面的 expect 逻辑
        break
    fi
    # 模拟等待：我们看到 CoreTime 打印出来后，杀掉 perf
    sleep 0.1
done

# 如果你手动操作，就在 CoreTime 出来的一瞬间，在终端按回车前，执行：
# kill $PERF_PID 

echo ">>> 核心循环已结束，正在提取纯净数据..."
kill $PERF_PID
wait $PERF_PID 2>/dev/null

# 5. 让程序彻底退出
echo "" > "$tmp_pipe"
rm "$tmp_pipe"

# 6. 展示结果
echo "--------------------------------"
cat core_perf.txt | grep -E "cycles|instructions|page-faults|LLC-load-misses"
echo "--------------------------------"