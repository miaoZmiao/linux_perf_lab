#!/bin/bash
# 检查是否安装了 bcc-tools
if ! command -v profile-bpfcc &> /dev/null; then
    echo "请先安装 bcc-tools: sudo apt install bpfcc-tools"
    exit 1
fi

echo "正在对系统进行 10 秒钟的 CPU 采样..."
echo "这能抓取所有进程的调用栈，找出耗时最高的函数。"

# 采样 10 秒，并生成折叠后的调用栈数据
sudo profile-bpfcc -F 99 10 > cpu_profile.txt

echo "采样完成！你可以查看 cpu_profile.txt"
echo "建议：使用火焰图工具将该文件转化为 svg 图片观看。"