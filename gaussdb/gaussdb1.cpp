#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <pthread.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <sys/syscall.h>
#include <map>
#include <queue>
#include <random> // 用于概率测试

// --- 宏和类型定义模拟 openGauss ---
#define GS_SIGNAL_COUNT 32 // 信号数量上限
#define RES_SIGNAL SIGUSR2   // 模拟 openGauss 的内部保留信号 (SIGUSR2)

using ThreadId = pthread_t; 
using gs_sigfunc = void (*)(int); 

// --- 全局变量和同步对象 ---
std::atomic<bool> g_shutdown(false);
std::mutex g_log_mutex;              // 保护 std::cout
std::mutex g_signal_base_lock;       // 模拟 g_instance.signal_base->slots_lock 

// 模拟 GsSignal: 线程内部的信号管理结构
struct GsSignal {
    std::queue<int> used_signals;      // 存储待处理信号的队列
    gs_sigfunc handlerList[GS_SIGNAL_COUNT] = {nullptr}; // 信号处理器数组
};

// 模拟 GsSignalSlot: 存储线程信息和其 GsSignal 结构
struct GsSignalSlot {
    ThreadId thread_id = 0;
    pid_t lwtid = 0; // Linux 内核线程 ID (LWP)
    GsSignal gssignal;
    std::thread::id cpp_tid;
};

// 存储所有线程槽位
std::map<ThreadId, GsSignalSlot*> g_signal_slots;

// 线程局部变量：指向当前线程的槽位
thread_local GsSignalSlot* t_thrd_signal_slot = nullptr;

// --- C 风格信号处理函数 (需求 1: 取消 Lambda) ---

void handle_sighup(int signo) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cout << "[WORKER KERNEL_TID:" << t_thrd_signal_slot->lwtid << "] SIGHUP Handler: Reloading configuration (simulated)." << std::endl;
}

void handle_sigterm_and_int(int signo) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cout << "[WORKER KERNEL_TID:" << t_thrd_signal_slot->lwtid << "] Received simulated signal " << signo 
              << " (" << strsignal(signo) << "). Preparing for graceful exit." << std::endl;
    g_shutdown.store(true);
}


// --- 实用工具函数 ---

pid_t get_kernel_tid() {
    return syscall(SYS_gettid);
}

ThreadId gs_thread_self() {
    return pthread_self();
}

void gs_signal_handle() {
    GsSignal* pGsSignal = &t_thrd_signal_slot->gssignal;
    int signum;
    
    while (!pGsSignal->used_signals.empty()) {
        signum = pGsSignal->used_signals.front();
        pGsSignal->used_signals.pop();

        if (signum >= 0 && signum < GS_SIGNAL_COUNT && pGsSignal->handlerList[signum] != nullptr) {
            
            // 调用用户注册的处理器
            pGsSignal->handlerList[signum](signum);
        }
    }
}

void gs_res_signal_handler(int signo) {
    if (signo == RES_SIGNAL) {
        gs_signal_handle(); 
    }
}

// --- openGauss 接口模拟 ---

void gs_signal_startup_siginfo() {
    ThreadId tid = gs_thread_self();
    
    if (t_thrd_signal_slot != nullptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(g_signal_base_lock);

    GsSignalSlot* new_slot = new GsSignalSlot();
    new_slot->thread_id = tid;
    new_slot->lwtid = get_kernel_tid();
    new_slot->cpp_tid = std::this_thread::get_id();
    
    for (int i = 0; i < GS_SIGNAL_COUNT; ++i) {
        new_slot->gssignal.handlerList[i] = nullptr;
    }

    g_signal_slots[tid] = new_slot;
    t_thrd_signal_slot = new_slot; 

    // 关键：只安装 RES_SIGNAL (SIGUSR2) 的处理器 
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gs_res_signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(RES_SIGNAL, &sa, nullptr); 

    std::lock_guard<std::mutex> log_lock(g_log_mutex);
    std::cout << "[INFO KERNEL_TID:" << new_slot->lwtid << "] Slot created and SIGUSR2 handler installed." << std::endl;
}

gs_sigfunc gspqsignal(int signo, gs_sigfunc func) {
    if (t_thrd_signal_slot == nullptr || signo < 0 || signo >= GS_SIGNAL_COUNT) {
        return nullptr; 
    }
    
    gs_sigfunc prefun = t_thrd_signal_slot->gssignal.handlerList[signo];
    t_thrd_signal_slot->gssignal.handlerList[signo] = func;
    
    return prefun;
}

int gs_signal_set_signal_by_threadid(ThreadId thread_id, int signo) {
    std::lock_guard<std::mutex> lock(g_signal_base_lock);
    
    auto it = g_signal_slots.find(thread_id);
    if (it == g_signal_slots.end()) {
        return ESRCH; 
    }
    
    GsSignalSlot* target_slot = it->second;
    target_slot->gssignal.used_signals.push(signo);
    
    return 0;
}

/**
 * @brief 模拟 gs_signal_send(): 向目标线程发送模拟信号。
 * 核心逻辑：大部分时间发送 SIGUSR2，小部分时间直接发送原始信号 (需求 2)。
 */
int gs_signal_send(ThreadId thread_id, int signo) {
    // 静态原子计数器，用于生成伪随机数
    static std::atomic<int> rand_counter(0);
    rand_counter++;
    
    // 【实验场景 2】：10% 的概率，直接使用 pthread_kill 发送原始信号。
    // 我们只对 SIGTERM, SIGHUP, SIGINT 进行此测试，因为它们的默认行为是终止/挂起。
    if ((signo == SIGTERM || signo == SIGHUP || signo == SIGINT) && (rand_counter.load() % 2 == 0)) {
        int code = pthread_kill(thread_id, signo);
        std::lock_guard<std::mutex> lock(g_log_mutex);
        if (code == 0) {
            std::cout << "\n[!!! TEST ALERT KERNEL_TID:" << get_kernel_tid() << " !!!]" 
                      << " Directly sent unqueued signal " << signo << " (" << strsignal(signo) 
                      << ") to ThreadId " << thread_id << ". **Default OS action will occur.**" << std::endl;
        } else {
             std::cerr << "\n[!!! TEST ERROR !!!] Direct kill failed: " << std::strerror(code) << std::endl;
        }
        return code;
    }


    // 1. 将信号记录到目标线程的内部队列
    if (gs_signal_set_signal_by_threadid(thread_id, signo) != 0) {
        return ESRCH;
    }
    
    // 2. 发送 RES_SIGNAL (SIGUSR2) 唤醒目标线程
    int code = pthread_kill(thread_id, RES_SIGNAL);
    
    if (code != 0) {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cerr << "[ERROR] Failed to send RES_SIGNAL to ThreadId " << thread_id 
                  << ": " << std::strerror(code) << std::endl;
    } else {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cout << "[INFO] Successfully sent and simulated signal " << signo 
                  << " to ThreadId " << thread_id << std::endl;
    }
    
    return code;
}


// --- 信号和工作线程实现 ---

void signal_waiter_thread(ThreadId worker_tid_to_notify) {
    sigset_t waitMask;
    sigemptyset(&waitMask);
    sigaddset(&waitMask, SIGINT);
    sigaddset(&waitMask, SIGTERM);
    sigaddset(&waitMask, SIGHUP);
    
    pid_t kernel_tid = get_kernel_tid();

    {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cout << "[SIGNAL_WAITER KERNEL_TID:" << kernel_tid << "] Starting. Waiting for external signals..." << std::endl;
    }

    while (!g_shutdown) {
        int signum;
        int ret = sigwait(&waitMask, &signum); 
        
        if (ret == 0) {
            std::string sig_name = strsignal(signum);
            
            {
                std::lock_guard<std::mutex> lock(g_log_mutex);
                std::cout << "\n[SIGNAL_WAITER KERNEL_TID:" << kernel_tid << "] Received external signal " 
                          << signum << " (" << sig_name << "). PID: " << getpid() << std::endl;
            }
            
            if (worker_tid_to_notify != 0) {
                gs_signal_send(worker_tid_to_notify, signum);
            }
            
            if (signum == SIGTERM || signum == SIGINT) {
                // 等待 worker 线程优雅处理退出标志
                break;
            }
        }
    }
    
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cout << "[SIGNAL_WAITER KERNEL_TID:" << kernel_tid << "] Exiting." << std::endl;
}

void worker_thread(int id) {
    // 1. 初始化线程信号信息
    gs_signal_startup_siginfo();

    pid_t kernel_tid = t_thrd_signal_slot->lwtid;
    
    // 2. 注册信号处理函数 (使用 C 风格函数)
    gspqsignal(SIGHUP, handle_sighup);
    gspqsignal(SIGTERM, handle_sigterm_and_int);
    gspqsignal(SIGINT, handle_sigterm_and_int);

    {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cout << "[WORKER " << id << " KERNEL_TID:" << kernel_tid << "] Executing task ..." << std::endl;
    }
    // 3. 核心业务循环
    size_t cnt = 0;
    while (!g_shutdown) {
        // 模拟执行任务
        if (cnt % 20 == 0) {
            // std::lock_guard<std::mutex> lock(g_log_mutex);
            // std::cout << "[WORKER " << id << " KERNEL_TID:" << kernel_tid << "] Executing task (" << cnt << ")..." << std::endl;
            cnt = 0;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        cnt++;
    }
    
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cout << "[WORKER " << id << " KERNEL_TID:" << kernel_tid << "] Shutting down. Final task count: " << cnt << std::endl;
}

/**
 * @brief 调整信号屏蔽字，确保 SIGTERM/SIGHUP/SIGINT 在工作线程中是非阻塞的，
 * 以便测试场景 2 (直接发送非 SIGUSR2 信号) 时能执行默认行为。
 */
void setup_signal_masks_for_experiment() {
    sigset_t set;
    sigfillset(&set); 

    // 这些信号在工作线程中必须是非阻塞的 (Unblocked)，才能在测试场景 2 中，
    // 当 gs_signal_send 直接调用 pthread_kill(..., SIGTERM) 时，线程能立即响应默认行为 (终止进程)。
    sigdelset(&set, SIGINT); 
    sigdelset(&set, SIGTERM);
    sigdelset(&set, SIGHUP);

    // RES_SIGNAL (SIGUSR2) 必须是非阻塞的，用于唤醒工作线程
    sigdelset(&set, RES_SIGNAL); 
    
    // 致命信号 (用于 core dump) 也应该是非阻塞的
    sigdelset(&set, SIGSEGV); 
    sigdelset(&set, SIGBUS); 
    
    int ret = pthread_sigmask(SIG_BLOCK, &set, nullptr);
    if (ret != 0) {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cerr << "[ERROR] Failed to set up signal mask: " << std::strerror(ret) << std::endl;
        exit(1);
    }
}

/**
 * @brief 在主线程中阻塞所有信号，确保只有 signal_waiter_thread 收到它们。
 */
void block_all_signals() {
    sigset_t set;
    sigfillset(&set); // 阻塞所有信号
    // RES_SIGNAL (SIGUSR2) 必须被排除在阻塞之外，因为工作线程需要接收它来触发 gs_signal_handle
    sigdelset(&set, RES_SIGNAL); 
    // SIGSEGV, SIGBUS 等致命信号也应该排除，以允许默认行为
    sigdelset(&set, SIGSEGV); 
    sigdelset(&set, SIGTERM);
    sigdelset(&set, SIGBUS); 
    
    // 为了实验需要，要把进程的屏蔽 SIGABRT行为放开
    sigdelset(&set, SIGABRT);
    int ret = pthread_sigmask(SIG_BLOCK, &set, nullptr);
    if (ret != 0) {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cerr << "[ERROR] Failed to block signals: " << std::strerror(ret) << std::endl;
        exit(1);
    }
}


int main() {
    // setup_signal_masks_for_experiment();
    block_all_signals();
    const int NUM_WORKERS = 3;
    std::vector<std::thread> threads;
    std::vector<ThreadId> worker_tids;
    
    srand(time(nullptr)); 

    pid_t main_kernel_tid = get_kernel_tid();

    {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cout << "gaussdb_sim started. PID: " << getpid() 
                  << ". Main KERNEL_TID: " << main_kernel_tid << ". Use 'kill -信号编号 " << getpid() << "' to test." << std::endl;
    }
    
    for (int i = 0; i < NUM_WORKERS; ++i) {
        threads.emplace_back(worker_thread, i + 1);
        worker_tids.push_back(threads.back().native_handle());
    }
    
    // 让信号接收专职线程转发给第一个工作线程
    threads.emplace_back(signal_waiter_thread, worker_tids.at(0));

    {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cout << "[MAIN KERNEL_TID:" << main_kernel_tid << "] Waiting for graceful shutdown..." << std::endl;
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    for (auto const& [tid, slot] : g_signal_slots) {
        delete slot;
    }

    {
        std::lock_guard<std::mutex> final_lock(g_log_mutex);
        std::cout << "[MAIN KERNEL_TID:" << main_kernel_tid << "] All threads finished. Exiting." << std::endl;
    }

    return 9;
}