#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <immintrin.h>

int main() {
    const int size = 10000000;
    // 使用 aligned_alloc 或者 vector（通常在现代系统上 vector 的 data() 是 32 字节对齐的）
    std::vector<long long> data(size);

    std::mt19937 rng(42); 
    std::uniform_int_distribution<long long> dist(0, 100);
    for (int i = 0; i < size; ++i) {
        data[i] = dist(rng);
    }

    long long final_sum = 0;
    
    // 准备常量寄存器
    __m256i limit = _mm256_set1_epi64x(50);
    __m256i zero = _mm256_setzero_si256();

    for (int tick = 0; tick < 100; ++tick) {
        __m256i sum_vec = _mm256_setzero_si256();
        
        for (int i = 0; i < size; i += 4) {
            // 一次性加载 4 个不同的元素：[d0, d1, d2, d3]
            __m256i val = _mm256_loadu_si256((__m256i*)&data[i]);

            // 比较：如果 50 > val，对应位置设为全 1
            __m256i mask = _mm256_cmpgt_epi64(limit, val);

            // 取反：0 - val
            __m256i neg_val = _mm256_sub_epi64(zero, val);

            // 选择：根据 mask 选择 val 或 neg_val
            // _mm256_blendv_epi8(a, b, mask): mask 为 1 选 b，0 选 a
            __m256i selected = _mm256_blendv_epi8(neg_val, val, mask);

            // 累加
            sum_vec = _mm256_add_epi64(sum_vec, selected);
        }

        // 将矢量寄存器中的 4 个 long long 横向累加
        long long res[4];
        _mm256_storeu_si256((__m256i*)res, sum_vec);
        final_sum += (res[0] + res[1] + res[2] + res[3]);
    }

    std::cout << "Sum: " << final_sum << std::endl;
    return 0;
}