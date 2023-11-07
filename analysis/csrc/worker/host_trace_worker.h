/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_trace_worker.h
 * Description        : HostTraceWorker模块：
 *                      负责拉起数据解析->EventQueue->建树->分析树->Dump 流程
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_WORKER_HOST_TRACE_THREAD_H
#define ANALYSIS_WORKER_HOST_TRACE_THREAD_H
namespace Analysis {
namespace Worker {

// HostTraceWorker负责控制CANN Host侧数据解析->EventQueue->建树->分析树->Dump 流程
// 此流程由Worker拉起
class HostTraceWorker {
public:
    // 启动整个流程
    bool Run();
};

} // namespace Worker
} // namespace Analysis
#endif // ANALYSIS_WORKER_HOST_TRACE_THREAD_H
