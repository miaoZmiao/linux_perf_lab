#!/bin/bash

# 检查权限
if [ "$EUID" -ne 0 ]; then
  echo "请使用 sudo 运行此脚本，因为 eBPF 需要 root 权限。"
  exit 1
fi

# 检查 bcc 工具是否安装
if ! command -v profile-bpfcc &> /dev/null; then
    echo "未发现 profile-bpfcc，请先安装 bcc-tools: sudo apt install bpfcc-tools"
    exit 1
fi

# 设置路径
CUR_DIR=$(cd $(dirname $0); pwd)
# 根据你的描述：脚本在 ./cpu/，可执行文件在 ./build/cpu/
BUILD_BIN_DIR="${CUR_DIR}/../build/cpu"

# 让用户选择要测试的程序
echo "请选择要运行的 CPU 压力程序："
echo "1) cpu_high_sys (高系统调用负载)"
echo "2) cpu_high_user (高用户态计算负载)"
echo "3) cpu_context_switch (高上下文切换负载)"
read -p "输入编号 [1-3]: " choice

case $choice in
    1) TARGET="cpu_high_sys" ;;
    2) TARGET="cpu_high_user" ;;
    3) TARGET="cpu_context_switch" ;;
    *) echo "无效选择"; exit 1 ;;
esac

BIN_PATH="${BUILD_BIN_DIR}/${TARGET}"

# 检查二进制文件是否存在
if [ ! -f "$BIN_PATH" ]; then
    echo "错误: 未找到可执行文件 $BIN_PATH"
    echo "请确保已经进入 build 目录执行过 make。"
    exit 1
fi

echo "================================================="
echo "正在启动进程: $TARGET"
echo "================================================="

# 1. 后台启动目标进程
$BIN_PATH &
PROG_PID=$!

# 确保进程启动成功
sleep 5
if ! kill -0 $PROG_PID 2>/dev/null; then
    echo "进程启动失败，请检查程序输出。"
    exit 1
fi

echo "进程已启动 (PID: $PROG_PID)，开始 eBPF 采样 (持续 10 秒)..."

# 2. 运行 eBPF 采样
# -F 99 表示每秒采样 99 次；10 表示持续 10 秒
# 我们只过滤出与该进程相关的堆栈
profile-bpfcc -F 99 10 -p $PROG_PID > cpu_profile.txt

echo "采样完成，正在清理进程..."

# 3. 杀掉压测进程
kill $PROG_PID
wait $PROG_PID 2>/dev/null

echo "================================================="
echo "               采样结果分析 (Top 10)              "
echo "================================================="
# 简单处理：提取出现频次最高的堆栈底部（通常是函数调用入口）
# grep -v "^    " cpu_profile.txt | grep -v "^$" | sort | uniq -c | sort -rn | head -n 10

echo -e "\n详细结果已保存至: $(pwd)/cpu_profile.txt"
echo "提示：你可以将该文件内容上传至火焰图生成器查看。"