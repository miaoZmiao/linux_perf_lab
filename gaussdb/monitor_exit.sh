#!/bin/bash

# 获取脚本所在的绝对路径
SCRIPT_DIR=$(cd $(dirname $0); pwd)
cd "$SCRIPT_DIR"

# 1. 定义二进制文件相对路径
# 脚本在 ./gaussdb/，目标在 ../build/gaussdb/gaussdb
BINARY="../build/gaussdb/gaussdb"

echo "[+] 当前工作目录: $PWD"

# 2. 检查并启动 gaussdb (后台运行)
if [ -f "$BINARY" ]; then
    # 赋予执行权限以防万一
    chmod +x "$BINARY"
    nohup "$BINARY" > gaussdb_runtime.log 2>&1 &
    GAUSS_PID=$!
    echo "[+] GaussDB 已启动, PID: $GAUSS_PID"
else
    echo "[-] 错误: 在 $BINARY 找不到执行文件"
    exit 1
fi

# 3. 启动 bpftrace 监控 (后台运行)
echo "[+] 正在启动 bpftrace 监控退出事件..."
# 使用 sudo 运行，并将输出重定向
sudo bpftrace -e '
kfunc:vmlinux:do_exit
{
    $leader_comm = curtask->group_leader->comm;

    if (strcontains($leader_comm, "gaussdb")) {
        $exit_code = args->code >> 8;
        $signal = args->code & 0xFF;
        
        printf("%s Group Leader:%-15s PID:%-6d TID:%-6d ThreadName:%-15s Exit:%-3d Sig:%-3d\n", 
               strftime("%Y-%m-%d %H:%M:%S", nsecs),
               $leader_comm, pid, tid, comm, $exit_code, $signal);
    }
}' > gaussdb_exit.log 2>&1 &

BPF_PID=$!
echo "[+] bpftrace 已后台运行, PID: $BPF_PID"

# 4. 等待用户输入信号
echo "------------------------------------------------"
echo "等待 GaussDB 运行中..."
read -p "请输入要发送给 GaussDB 的信号值 (例如输入 9 则执行 kill -9): " SIG_VAL

if [[ "$SIG_VAL" =~ ^[0-9]+$ ]]; then
    echo "[!] 执行: kill -$SIG_VAL $GAUSS_PID"
    kill -$SIG_VAL $GAUSS_PID
    
    # 5. 等待 1s
    sleep 1
    
    # 6. 停止 bpftrace
    echo "[+] 正在停止监控并保存日志..."
    # 使用 SIGINT (2) 停止 bpftrace 能够确保其刷新缓冲区完成最后的 printf 输出
    sudo kill -2 $BPF_PID
    
    echo "------------------------------------------------"
    echo "[√] 操作完成。"
    echo "[i] GaussDB 运行日志: ./gaussdb/gaussdb_runtime.log"
    echo "[i] BPF 退出监控日志: ./gaussdb/gaussdb_exit.log"
else
    echo "[-] 输入无效，脚本退出。请手动清理后台进程。"
fi