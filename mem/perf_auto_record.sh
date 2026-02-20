#!/bin/bash
# 用法: ./perf_auto_record.sh shuffle warmup

# 1. 启动程序
tmp_pipe=$(mktemp -u)
mkfifo "$tmp_pipe"
./mem/mem_seq_vs_rand $1 $2 < "$tmp_pipe" &
APP_PID=$!
sleep 1

# 2. 开启 record 采样 (用户态，带调用栈)
perf record -p $APP_PID -e cycles:u -g -o perf.data -- sleep 100 &
RECORD_PID=$!
sleep 1

# 3. 运行核心循环
echo "" > "$tmp_pipe"

# 4. 监测循环结束 (观察 CoreTime 是否打印)
# 简单做法：等待程序结束或手动杀掉 record
echo "等待循环完成..."
# 这里我们通过等待程序进入第二个门闩来判断
sleep 2 

# 5. 立即停止采样，防止抓到 munmap
kill -SIGINT $RECORD_PID
wait $RECORD_PID 2>/dev/null

echo ">>> 采集已完成！执行 'perf report' 查看热点。"
kill -9 $APP_PID 2>/dev/null
rm "$tmp_pipe"