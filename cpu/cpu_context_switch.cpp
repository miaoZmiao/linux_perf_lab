#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h> // 包含 usleep

std::mutex mtx;

void switch_task() {
    while (true) {
        // 频繁请求锁并立即释放，诱发内核进行线程调度
        std::lock_guard<std::mutex> lock(mtx);
        // 让出 CPU 时间片，强制内核进行上下文切换
        // usleep(1);
        std::this_thread::yield(); 
    }
}

int main() {
    // 启动远超核心数的线程，例如 500 个
    const int thread_count = 500;
    std::cout << "Starting " << thread_count << " threads to trigger context switches..." << std::endl;

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(switch_task);
    }

    for (auto& t : threads) t.join();
    return 0;
}