/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : thread_pool.h
 * Description        : ThreadPool：提供线程安全的线程池功能
 * Author             : msprof team
 * Creation Date      : 2023/11/6
 * *****************************************************************************
 */

#ifndef ANALYSIS_UTILS_THREAD_POOL_H
#define ANALYSIS_UTILS_THREAD_POOL_H

#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <deque>
#include <vector>

namespace Analysis {
namespace Utils {

// 线程池中Task类型定义
using Task = std::function<void()>;

// 线程池
class ThreadPool {
public:
    explicit ThreadPool(uint32_t threadsNum = 1);
    // 禁止生成拷贝构造函数
    ThreadPool(const ThreadPool &) = delete;
    // 禁止生成拷贝赋值运算符
    ThreadPool &operator=(const ThreadPool &other) = delete;
    // 禁止生成移动构造函数
    ThreadPool(ThreadPool &&other) = delete;
    // 禁止生成移动构造函数
    ThreadPool &operator=(ThreadPool &&other) = delete;
    // 析构时自动释放所有资源
    ~ThreadPool();
    // 启动线程池，创建threadsNum个线程等待任务
    bool Start();
    // 停止线程池(停止任务分配，获得任务的线程待其执行完毕), 最终释放所有线程
    bool Stop();
    // 等待任务队列中的任务分配到线程去执行
    // 此函数不保证任务执行完毕, 若要实现等待所有任务执行结束需要再调用Stop
    void WaitAllTasks();
    // 向任务队列中添加一个任务
    void AddTask(const Task &task);
    // 获取未分配给线程执行的任务数量（任务队列的实际大小）
    uint32_t GetUnassignedTasksNum();
    // 获取线程数量
    uint32_t GetThreadsNum();

private:
    // 每个线程默认执行的循环函数
    void Loop();
    // 从任务队列中获取一个任务，成功返回true
    bool FetchTask(Task &task);

private:
    std::mutex mutex_;
    std::condition_variable hasTaskToDo_;
    std::condition_variable waitTaskDone_;
    std::vector<std::thread> threads_;
    std::deque<Task> taskQueue_;
    uint32_t threadsNum_;
    bool running_ = false;
};

} // namespace Utils
} // namespace Analysis
#endif // ANALYSIS_UTILS_THREAD_POOL_H
