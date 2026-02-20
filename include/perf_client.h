#ifndef PERF_CLIENT_H
#define PERF_CLIENT_H

#include <sys/types.h>

// #ifdef __cplusplus
// extern "C" {
// #endif

// 开启录制：发送 START 信号给后台 Server
void perf_record_start(pid_t pid, const char* name);

// 停止录制：发送 STOP 信号给后台 Server
void perf_record_stop(pid_t pid);

// #ifdef __cplusplus
// }
// #endif

#endif