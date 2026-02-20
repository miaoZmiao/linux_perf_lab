#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

struct PerfSession {
    pid_t perf_pid;
    std::string output_file;
};

class PerfAuditServer {
private:
    int server_fd;
    std::map<pid_t, PerfSession> sessions;
    const char* socket_path = "/tmp/perf_audit.sock";

public:
    PerfAuditServer() {
        unlink(socket_path);
        server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        sockaddr_un addr{.sun_family = AF_UNIX};
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
        bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
        listen(server_fd, 5);
    }

    void run() {
        std::cout << "[Server] 性能审计服务端已启动，监听中..." << std::endl;
        while (true) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            handle_client(client_fd);
            close(client_fd);
        }
    }

private:
    void handle_client(int fd) {
        char buffer[256];
        int n = read(fd, buffer, sizeof(buffer)-1);
        if (n <= 0) return;
        buffer[n] = '\0';

        char cmd[16], filename_orig[128];
        pid_t target_pid;
        sscanf(buffer, "%s %d %s", cmd, &target_pid, filename_orig);

        if (std::string(cmd) == "START") {
            start_perf(target_pid, filename_orig);
        } else if (std::string(cmd) == "STOP") {
            stop_perf(target_pid);
        }
    }

    void start_perf(pid_t target_pid, const char* file) {
        pid_t pid = fork();
        if (pid == 0) { // 子进程
            char pid_str[16];
            sprintf(pid_str, "%d", target_pid);
            execlp("perf", "perf", "record", "-e", "cycles:u", "-g", "-p", pid_str, "-o", file, nullptr);
            exit(0);
        } else {
            sessions[target_pid] = {pid, file};
            std::cout << "[Server] 开始录制 PID: " << target_pid << " -> " << file << std::endl;
        }
    }

    void stop_perf(pid_t target_pid) {
        if (sessions.count(target_pid)) {
            kill(sessions[target_pid].perf_pid, SIGINT);
            waitpid(sessions[target_pid].perf_pid, nullptr, 0);
            std::cout << "[Server] 停止录制 PID: " << target_pid << std::endl;
            sessions.erase(target_pid);
        }
    }
};

int main() {
    PerfAuditServer server;
    server.run();
    return 0;
}