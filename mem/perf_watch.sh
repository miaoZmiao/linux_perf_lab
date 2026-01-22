#!/bin/bash

# 1. 预备工作：清理缓存，确保从磁盘真实读取
echo "正在清理系统缓存，请稍候..."
sudo sync; echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null

# 2. 启动后台监控：sar (内存缺页) 和 iostat (磁盘IO)
# 每 1 秒采集一次，输出到临时文件
sar -B 1 10 > sar_tmp.txt &
SAR_PID=$!
iostat -dx 1 10 > iostat_tmp.txt &
IOSTAT_PID=$!

echo "------------------------------------------------"
echo "开始读取 4GB 文件 (cat test_file)..."
echo "你可以观察磁盘指示灯或等待几秒..."
echo "------------------------------------------------"

# 3. 执行压力测试并计时
# 如果没有文件，直接本地执行 
# dd if=/dev/urandom of=test_file bs=1M count=4096
# 文件就有了
time cat test_file > /dev/null

# 4. 停止后台监控
kill $SAR_PID $IOSTAT_PID 2>/dev/null

# 5. 提取并展示关键结论
echo -e "\n[实验数据分析汇总]"
echo "------------------------------------------------"
echo "1. 内存缺页情况 (sar -B):"
# 提取带有数据的行（排除表头和空行）
grep -A 1 "majflt/s" sar_tmp.txt | tail -n 2

echo -e "\n2. 磁盘吞吐情况 (iostat):"
# 自动定位你的主要磁盘（通常是 sda, sdb 或 nvme0n1）
grep -E "Device|sd|nvme" iostat_tmp.txt | grep -v "loop" | head -n 5

echo -e "\n3. 缓存填充情况 (free):"
free -h
echo "------------------------------------------------"

# 清理临时文件
rm sar_tmp.txt iostat_tmp.txt