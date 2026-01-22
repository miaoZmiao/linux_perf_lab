#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>
#include <unistd.h>

// 模拟一个不断增长的全局缓存（由于代码逻辑漏洞，只加不减）
std::vector<char*> g_leak_registry;

void leak_step(size_t mb_size) {
    // 申请内存
    size_t bytes = mb_size * 1024 * 1024;
    char* buf = new char[bytes];
    
    // 关键：必须初始化（写入），内核才会真正分配物理页面 (RES)
    // 否则只是增加了虚拟地址空间 (VIRT)
    std::memset(buf, 0xBB, bytes);
    
    // 存入全局变量，模拟“忘记释放”
    g_leak_registry.push_back(buf);
}

int main() {
    std::cout << "Memory Leak Experiment Started. PID: " << getpid() << std::endl;
    std::cout << "每秒将泄漏 10MB 内存..." << std::endl;

    while (true) {
        leak_step(10); // 每次泄漏 10MB
        
        if (g_leak_registry.size() % 10 == 0) {
            std::cout << "已累积泄漏: " << g_leak_registry.size() * 10 << " MB" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}