#!/bin/bash
rm -f sar_dev.log
# 1. 准备：清除之前的 tc 限制，确保带宽纯净
sudo tc qdisc del dev lo root 2>/dev/null

echo "================================================="
echo "开始测试：本地回环流量饱和度测试"
echo "目标：观察 rxkB/s, txkB/s 和 %ifutil 的极限"
echo "================================================="

# 2. 启动监控
sar -n DEV 1 10 > sar_dev.log &  # 监控设长一点
SAR_PID=$!

# 3. 产生大量流量 (持续约 5-8 秒)
# 使用 python 启动一个丢弃数据的接收端，然后用 dd 疯狂发送
nc -l -p 9999 > /dev/null &
NC_PID=$!
sleep 1

echo "正在疯狂灌入流量..."
dd if=/dev/zero bs=1M count=2000 | nc -N 127.0.0.1 9999

# 4. 汇总结果
# echo "wait ${SAR_PID}..."
wait $SAR_PID
kill $NC_PID 2>/dev/null

echo -e "\n[网卡流量数据汇总]"
echo "-------------------------------------------------"
# 过滤并显示本地回环网卡 lo 的最高值
# grep "lo" sar_dev.log | grep -v "IFACE" | sort -k5 -n | tail -n 5

# 1. 打印表头
grep "IFACE" sar_dev.log | head -n 1
# 2. 过滤 lo 网卡，且只显示 rxpck/s 大于 100 的行（过滤掉空闲时间），按时间排序
grep "lo" sar_dev.log | awk '$3 > 100' | sort -k1

echo -e "\n[诊断指南]"
echo "1. 看 rxkB/s & txkB/s：这是你的虚拟网卡每秒搬运的数据量（通常能到 GB 级别）。"
echo "2. 看 %ifutil：对于物理网卡，如果这个值接近 100%，你的网络就开始排队了。"
echo "3. 对比：你会发现 lo 网卡的 %ifutil 永远是 0，因为它是虚拟的，没有硬件上限。"

# rm sar_dev.log