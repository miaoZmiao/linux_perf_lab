#include <unistd.h>
#include <iostream>
#include <vector>
#include <thread>

void high_sys_task() {
    char buf[10];
    while (true) {
        // 频繁调用系统调用 read
        // 从 /dev/zero 读数据虽然很快，但每次调用都要进出内核
        read(0, buf, 0); 
    }
}

int main() {
    unsigned int n = std::thread::hardware_concurrency();
    std::cout << "Starting system-call stress on " << n << " threads..." << std::endl;

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < n; ++i) {
        threads.emplace_back(high_sys_task);
    }

    for (auto& t : threads) t.join();
    return 0;
}