#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "perf_client.h"

// 辅助函数：分配大页或普通内存
int* allocate_memory(size_t size, bool use_huge_pages) {
    size_t byte_size = size * sizeof(int);
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    if (use_huge_pages) {
        flags |= MAP_HUGETLB;
    }

    void* ptr = mmap(NULL, byte_size, PROT_READ | PROT_WRITE, flags, -1, 0);
    
    if (ptr == MAP_FAILED) {
        if (use_huge_pages) {
            std::cerr << "[Warning] Huge Pages allocation failed! Check /proc/meminfo. Falling back to normal pages." << std::endl;
            return allocate_memory(size, false);
        } else {
            perror("mmap failed");
            exit(1);
        }
    }
    return static_cast<int*>(ptr);
}


void warmup(void* ptr, size_t size) {
    // 仅仅触碰每个 Page 的第一个字节，建立 TLB 映射即可
    // 不要做连续的线性读取，避免触发预取器的“防过载”
    char* data = (char*)ptr;
    for (size_t j = 0; j < size; j += 4096) { // 按页跳跃
        volatile char temp = data[j];
        (void)temp;
    }
    // 给 CPU 50ms 的喘息时间，让预取器状态机复位
    usleep(50000); 
}

int main(int argc, char** argv) {
    const size_t SIZE = 128 * 1024 * 1024; 
    bool do_shuffle = false;
    bool do_warmup = false;
    bool use_huge = false;
    bool use_prefetch = false;
    // 解析参数
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if(arg == "shuffle") do_shuffle = true;
        if(arg == "warmup") do_warmup = true;
        if(arg == "huge") use_huge = true;
        if(arg == "prefetch") use_prefetch = true;
    }

    // 1. 分配内存 (data 数组使用 mmap 模拟真实数据库分配)
    int* data = allocate_memory(SIZE, use_huge);
    // 显式初始化 data，确保页表在此时“初步建立”
    std::fill(data, data + SIZE, 0);

    // 2. 准备索引数组
    std::vector<size_t> indices(SIZE);
    std::iota(indices.begin(), indices.end(), 0);
    if (do_shuffle) {
        std::srand(42);
        std::random_shuffle(indices.begin(), indices.end());
    }

    // 3. 预热逻辑 (单因素验证：只热索引和建立完整映射)
    if (do_warmup) {
        // volatile size_t tmp = 0;
        // // 预热索引表并触碰 data，消除所有潜在 Page Fault
        // size_t s_end = SIZE >> 5; // 预热前 1/32 的数据，足以建立完整映射
        // for (size_t i = 0; i < s_end; ++i) {
        //     tmp = indices[i];
        //     tmp = data[tmp]; 
        // }
        warmup(data, SIZE);
        std::cout << "[Info] Warmup complete (Indices & Data touched)." << std::endl;
    }

    pid_t my_pid = getpid();
    std::cout << "PID: " << my_pid << " | Mode: " 
              << (do_shuffle ? "Shuffle" : "Sequential") 
              << (use_huge ? " + HugePages" : " + NormalPages")
              << " | Press Enter to start..." << std::endl;
    // std::cin.get(); // 根据需要开启

    // --- 核心开始：自动化审计 ---
    std::string mode_str = (do_shuffle ? "shuffle" : "seq");
    std::string page_str = (use_huge ? "_huge" : "_norm");
    std::string report_name = "perf_" + mode_str + page_str + "_" + std::to_string(my_pid) + ".data";
    
    perf_record_start(my_pid, report_name.c_str()); 

    auto start = std::chrono::high_resolution_clock::now();
    long long sum = 0;
    

    if (use_prefetch) {

        // 尝试手动展开循环并进行多路预取
        // size_t lookahead = 64; // 预取深度，需要根据实验调整
        // const int batch_size = 8;
        // size_t i = 0;
        // for (; i + batch_size < SIZE; i += batch_size) {
        //     // 第一步：批量预取索引对应的 data
        //     for (int j = 0; j < batch_size; ++j) {
        //         __builtin_prefetch(&data[indices[i + j + lookahead]], 0, 3);
        //     }
        //     // 第二步：执行当前的计算
        //     for (int j = 0; j < batch_size; ++j) {
        //         sum += data[indices[i + j]];
        //     }
        // }
        // for (i = 0; i < SIZE; i += 16) {
        //     // 同时开启 16 路预取，压榨内存控制器的并行度 (MLP)
        //     for (int j = 0; j < 16; ++j) 
        //         __builtin_prefetch(&data[indices[i + j + 128]], 0, 3);
            
        //     // 紧接着处理 16 个计算
        //     for (int j = 0; j < 16; ++j)
        //         sum += data[indices[i + j]];
        // }

        for (size_t i = 0; i < SIZE; i += 8) {
            // 同时发起 8 个预取，尝试强行占领所有 LFB 坑位
            __builtin_prefetch(&data[indices[i + 64]], 0, 3);
            __builtin_prefetch(&data[indices[i + 65]], 0, 3);
            __builtin_prefetch(&data[indices[i + 66]], 0, 3);
            __builtin_prefetch(&data[indices[i + 67]], 0, 3);
            __builtin_prefetch(&data[indices[i + 68]], 0, 3);
            __builtin_prefetch(&data[indices[i + 69]], 0, 3);
            __builtin_prefetch(&data[indices[i + 70]], 0, 3);
            __builtin_prefetch(&data[indices[i + 71]], 0, 3);
            // 剩下的 8 个计算
            sum += data[indices[i]];
            sum += data[indices[i + 1]];
            sum += data[indices[i + 2]];
            sum += data[indices[i + 3]];
            sum += data[indices[i + 4]];
            sum += data[indices[i + 5]];
            sum += data[indices[i + 6]];
            sum += data[indices[i + 7]];
            // ...以此类推
        }

    } else {
        // 核心循环
        // for (size_t i = 0; i < SIZE; i += 4) {
        //     sum += data[i];     // 活 1
        //     sum += data[i + 1]; // 活 2
        //     sum += data[i + 2]; // 活 3
        //     sum += data[i + 3]; // 活 4
        // }
        for (size_t i = 0; i < SIZE; ++i) {
            sum += data[indices[i]]; // <--- 必须用 indices 才能触发真随机访问
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    
    perf_record_stop(my_pid); 
    // --- 核心结束 ---

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Result: " << sum << " | CoreTime: " << duration << "ms" << std::endl;
    std::cout << "Data saved to: " << report_name << std::endl;

    munmap(data, SIZE * sizeof(int));
    return 0;
}