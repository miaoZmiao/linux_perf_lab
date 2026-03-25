#include "stub.h"
#include <stdio.h>

int main() {
    // 这是一个永远为假的条件
    int condition = 0;

    if (condition) {
        // O0 下，编译器会为这一行生成调用指令，尝试寻找符号
        // O2 下，编译器发现 condition 恒为 0，直接删除这段代码
        error_link_failure_missing_symbol();
    } else {
        printf("Optimization is working!\n");
    }

    return 0;
}