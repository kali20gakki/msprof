/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : kernel_parser_worker.h
 * Description        : KernelParserWorker类，启动解析任务并跟踪任务结果
 * Author             : msprof team
 * Creation Date      : 2023-11-03
 * *****************************************************************************
 */

#ifndef ANALYSIS_WORKER_KERNEL_PARSER_WORKER_H
#define ANALYSIS_WORKER_KERNEL_PARSER_WORKER_H
#include <string>
#include <atomic>
#include <vector>

namespace Analysis {
namespace Worker {
// 解析流程控制类，传入host data所在路径，启动采集流程并返回结果
class KernelParserWorker {
public:
    // 初始化传入host路径
    explicit KernelParserWorker(std::string  hostFilePath);
    // 启动流程
    int Run();

private:
    // hashData解析落盘
    void DumpHashData();
    // typeInfo解析落盘
    void DumpTypeInfoData();
    // 启动CANN侧数据解析及分析流程
    void LaunchTraceParser();
    std::string hostFilePath_;
    // 用于子线程上报执行结果
    std::atomic<bool> result_;
};
} // Worker
} // Analysis
#endif // ANALYSIS_WORKER_KERNEL_PARSER_WORKER_H