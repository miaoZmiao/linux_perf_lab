#include <atomic>
#include <thread>
#include <vector>

struct GoodData {
    alignas(64) std::atomic<long long> count1{0};
    alignas(64) std::atomic<long long> count2{0};
} data; // 每个变量独占一个 64 字节的缓存行

void task1() {
    for (long long i = 0; i < 1000000000; ++i) data.count1++;
}
void task2() {
    for (long long i = 0; i < 1000000000; ++i) data.count2++;
}

int main() {
    std::thread t1(task1);
    std::thread t2(task2);
    t1.join(); t2.join();
    return 0;
}