#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>

void send_to_server(const std::string& msg) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    sockaddr_un addr{.sun_family = AF_UNIX};
    strcpy(addr.sun_path, "/tmp/perf_audit.sock");
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    write(fd, msg.c_str(), msg.length());
    close(fd);
}

void perf_record_start(pid_t pid, const char* name) {
    std::string msg = "START " + std::to_string(pid) + " " + name;
    send_to_server(msg);
    sleep(1); // 模拟一些准备时间  
}

void perf_record_stop(pid_t pid) {
    std::string msg = "STOP " + std::to_string(pid) + " placeholder";
    send_to_server(msg);
    sleep(1); // 模拟一些准备时间  
}