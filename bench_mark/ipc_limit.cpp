#include <iostream>
#include <chrono>

int main() {
    // 迭代次数足够多，保证能被 perf 捕捉到
    unsigned long long iterations = 1000000000; 

    std::cout << "Starting IPC limit test... Loop iterations: " << iterations << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    // 核心汇编块
    // 我们手动展开了循环，并在一次迭代中放入 8 组独立的加法
    // 每组加法操作不同的寄存器，完全消除数据依赖
    for (unsigned long long i = 0; i < iterations; ++i) {
        asm volatile (
            "add $1, %%rax\n\t"
            "add $1, %%rbx\n\t"
            "add $1, %%rcx\n\t"
            "add $1, %%rdx\n\t"
            "add $1, %%rsi\n\t"
            "add $1, %%rdi\n\t"
            "add $1, %%r8\n\t"
            "add $1, %%r9\n\t"
            :
            :
            : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9"
        );
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Done! CoreTime: " << duration << "ms" << std::endl;

    return 0;
}