#include <iostream>
#include <thread>
#include <vector>
#include <vector>

// 模拟计算密集型任务
void high_user_task() {
    std::cout << "Thread " << std::this_thread::get_id() << " started." << std::endl;
    long long count = 0;
    while (true) {
        count++; // 简单的自增，让 CPU 停不下来
        if (count > 1000000000) count = 0; 
    }
}

int main() {
    unsigned int n = std::thread::hardware_concurrency();
    std::cout << "Detected " << n << " CPU cores. Starting stress threads..." << std::endl;

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < n; ++i) {
        threads.emplace_back(high_user_task);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}