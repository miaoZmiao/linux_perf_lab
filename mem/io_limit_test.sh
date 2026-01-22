#!/bin/bash

TEST_FILE="io_limit_test_file"

echo "================================================="
echo "阶段 1：持续写入性能测试 (512MB 连续写入)"
echo "这个测试能看出你的 USB 硬盘写入带宽和延迟极限"
echo "================================================="

# 开启后台 iostat 监控
iostat -dx 1 15 > io_limit_stat.txt &
IOSTAT_PID=$!

# 执行大文件写入，并强制同步到磁盘 (fsync)
time dd if=/dev/zero of=$TEST_FILE bs=1M count=512 conv=fsync

echo -e "\n================================================="
echo "阶段 2：混合读写测试 (一边读一边写)"
echo "模拟最容易导致系统卡顿的场景"
echo "================================================="

# 后台读，前台写
cat test_file > /dev/null &
READ_PID=$!
dd if=/dev/zero of=${TEST_FILE}_2 bs=1M count=256 conv=fsync

# 等待读操作结束
wait $READ_PID
kill $IOSTAT_PID 2>/dev/null

echo -e "\n[极限性能数据汇总]"
echo "-------------------------------------------------"
echo "主要磁盘 (sda) 的响应数据："
# 提取写入期间的 iostat 记录（过滤掉初始空闲行）
grep "sda" io_limit_stat.txt | awk '$9 > 0 || $4 > 100' | head -n 20

echo -e "\n[分析指南]"
echo "1. 看 w_await：如果超过 500ms，说明你写入时系统会严重卡死。"
echo "2. 看 %util：如果长时间 100%，说明 USB 带宽已跑满。"
echo "3. 看 wkB/s：这就是你这块移动硬盘的真实写入天花板。"

# 清理
rm $TEST_FILE ${TEST_FILE}_2