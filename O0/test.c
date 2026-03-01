#include <stdio.h>

#define MOCK_BUG(flags) \
    asm volatile("ud2\n\t" \
                 ".pushsection .bug_table,\"a\"\n\t" \
                 ".long %c0\n\t" /* 使用 %c0 保证生成的汇编正确 */ \
                 ".popsection" \
                 : : "i" (flags))

int main() {
    int x = 10;
#ifdef O2
    // 在 O2 下折叠 x > 5 为 1
    MOCK_BUG(x > 5); 
#elif defined(O0)
    // 在 O0 下让 x > 5 无法折叠
    MOCK_BUG(x > 5); 
#endif
    return 0;
}