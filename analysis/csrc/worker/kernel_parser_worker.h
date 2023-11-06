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
namespace Analysis {
namespace Worker {
// 解析流程控制类，传入host data所在路径，启动采集流程并返回结果
class KernelParserWorker {
public:
    explicit KernelParserWorker(const std::string& hostFilePath);
    // 启动流程
    int Run();

private:
    std::string _hostFilePath;
    // 启动hash数据解析
    int LaunchHashParser();
    // 启动CANN侧数据解析及分析流程
    int LaunchTraceParser();
};
} // Worker
} // Analysis
#endif // ANALYSIS_WORKER_KERNEL_PARSER_WORKER_H