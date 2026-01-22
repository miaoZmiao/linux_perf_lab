#!/bin/bash
rm -f sar_net.log

# 1. 初始化环境
echo "正在配置网络环境：300ms 延迟 + 10% 丢包..."
sudo tc qdisc del dev lo root 2>/dev/null
sudo tc qdisc add dev lo root netem delay 300ms loss 10%

# 启动本地服务
python3 -m http.server 8080 > /dev/null 2>&1 &
SERVER_PID=$!
sleep 1

echo "------------------------------------------------"
echo "开始一键监控测试 (持续 10 秒)..."
echo "监控项：TCP 重传 (sar)、应用层响应 (curl)"
echo "------------------------------------------------"

# 2. 启动后台异步监控
sar -n ETCP 1 10 > sar_net.log &
SAR_PID=$!

# 3. 执行应用层压测 (10次请求)
# 我们把结果记录下来，看看到底几个成功，几个失败
SUCCESS=0
FAILURE=0

for i in {1..10}
do
    # 设置 3 秒超时，防止脚本卡死
    if curl -I -m 3 http://127.0.0.1:8080 > /dev/null 2>&1; then
        echo "请求 $i: [成功]"
        ((SUCCESS++))
    else
        echo "请求 $i: [失败]"
        ((FAILURE++))
    fi
done

# 等待监控数据收集完毕
wait $SAR_PID

# 4. 汇总展示报告
echo -e "\n================ 实验报告汇总 ================"
echo "应用层结果: 成功 $SUCCESS 次, 失败 $FAILURE 次"
echo "------------------------------------------------"
echo "内核层 TCP 指标 (sar 统计出的重传与失败):"
# 过滤掉表头，只看有数据的行
grep -E "retrans/s|平均时间|Average" sar_net.log | tail -n 2
echo "------------------------------------------------"

# 5. 现场清理
echo "正在恢复网络环境并关闭服务..."
sudo tc qdisc del dev lo root
kill $SERVER_PID
# rm -f sar_net.log
echo "实验结束。"