#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <immintrin.h>

int main() {
    const int size = 10000000;
    std::vector<long long> data(size);

    std::mt19937 rng(42); 
    std::uniform_int_distribution<long long> dist(0, 100);
    for (int i = 0; i < size; ++i) {
        data[i] = dist(rng);
    }

    long long final_sum = 0;
    
    __m256i limit = _mm256_set1_epi64x(50);
    __m256i zero = _mm256_setzero_si256();

    for (int tick = 0; tick < 100; ++tick) {
        __m256i sum_vec1 = _mm256_setzero_si256();
        __m256i sum_vec2 = _mm256_setzero_si256();
        
        for (int i = 0; i < size; i += 8) {
            // --- 第一组 ---
            __m256i val1  = _mm256_loadu_si256((__m256i*)&data[i]);
            __m256i mask1 = _mm256_cmpgt_epi64(limit, val1);
            __m256i neg1  = _mm256_sub_epi64(zero, val1);
            
            // 使用位运算替代 _mm256_blendv_epi8
            __m256i sel1  = _mm256_or_si256(
                                _mm256_and_si256(mask1, val1), 
                                _mm256_andnot_si256(mask1, neg1)
                            );
            sum_vec1 = _mm256_add_epi64(sum_vec1, sel1);

            // --- 第二组 ---
            __m256i val2  = _mm256_loadu_si256((__m256i*)&data[i+4]);
            __m256i mask2 = _mm256_cmpgt_epi64(limit, val2);
            __m256i neg2  = _mm256_sub_epi64(zero, val2);
            
            // 同上
            __m256i sel2  = _mm256_or_si256(
                                _mm256_and_si256(mask2, val2), 
                                _mm256_andnot_si256(mask2, neg2)
                            );
            sum_vec2 = _mm256_add_epi64(sum_vec2, sel2);
        }

        __m256i combined_sum = _mm256_add_epi64(sum_vec1, sum_vec2);
        long long res[4];
        _mm256_storeu_si256((__m256i*)res, combined_sum);
        final_sum += (res[0] + res[1] + res[2] + res[3]);
    }

    std::cout << "Sum: " << final_sum << std::endl;
    return 0;
}