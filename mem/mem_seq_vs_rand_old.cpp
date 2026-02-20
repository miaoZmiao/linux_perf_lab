#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <string>
#include <unistd.h>
#include "perf_client.h" // 1. 引入新定义的公共头文件

int main(int argc, char** argv) {
    const size_t SIZE = 128 * 1024 * 1024; 
    std::vector<int> data(SIZE);
    std::vector<size_t> indices(SIZE);
    std::iota(indices.begin(), indices.end(), 0);

    bool do_shuffle = false;
    bool do_warmup = false;

    for(int i=1; i<argc; ++i) {
        std::string arg = argv[i];
        if(arg == "shuffle") do_shuffle = true;
        if(arg == "warmup") do_warmup = true;
    }

    if (do_shuffle) {
        std::random_shuffle(indices.begin(), indices.end());
    }

    if (do_warmup) {
        // 专门预热索引表。因为 data 是随机访问，预热它没用（会被挤掉）
        // 但索引表是顺序访问，预热它能保证取地址的操作永远是 1 周期
        volatile size_t tmp = 0;
        for (size_t i = 0; i < SIZE; ++i) {
            tmp = indices[i];
        }
    }
    pid_t my_pid = getpid();
    std::cout << "PID: " << my_pid << " | Ready. Press Enter to start..." << std::endl;
    // std::cin.get(); 

    // --- 核心开始：自动化审计 ---
    // 通过动态库接口通知后台 perf_server，生成的采样文件以模式命名
    std::string report_name = std::string("perf_") + (do_shuffle ? "shuffle" : "seq") + "_" + std::to_string(my_pid) + ".data";
    perf_record_start(my_pid, report_name.c_str()); 

    auto start = std::chrono::high_resolution_clock::now();
    long long sum = 0;
    
    for (size_t i = 0; i < SIZE; ++i) {
        sum += data[indices[i]];
    }

    auto end = std::chrono::high_resolution_clock::now();
    
    perf_record_stop(my_pid); // 停止采样
    // --- 核心结束 ---

    std::cout << "Result: " << sum << " | CoreTime: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() 
              << "ms" << std::endl;

    std::cout << "Data saved to: " << report_name << std::endl;
    return 0;
}