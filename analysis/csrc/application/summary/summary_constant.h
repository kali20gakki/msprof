/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : summary_constant.h
 * Description        : 导出summary常量
 * Author             : msprof team
 * Creation Date      : 2024/11/30
 * *****************************************************************************
 */
#ifndef ANALYSIS_APPLICATION_SUMMARY_CONSTANT_H
#define ANALYSIS_APPLICATION_SUMMARY_CONSTANT_H

#include <string>
#include <map>
#include <cstdint>

namespace Analysis {
namespace Application {
const uint16_t  KILOBYTE = 1024;
const uint8_t ASSEMBLE_FAILED = 0;
const uint8_t ASSEMBLE_SUCCESS = 1;
const uint8_t DATA_NOT_EXIST = 2;
const std::string  UNKNOWN = "Unknown";
// file name
const std::string OP_SUMMARY_NAME = "op_summary";
const std::string NPU_MEMORY_NAME = "npu_mem";
const std::string NPU_MODULE_MEMORY_NAME = "npu_module_mem";
const std::string API_STATISTIC_NAME = "api_statistic";
const std::string FUSION_OP_NAME = "fusion_op";
const std::string TASK_TIME_SUMMARY_NAME = "task_time";
const std::string STEP_TRACE_SUMMARY_NAME = "step_trace";
const std::string COMM_STATISTIC_NAME = "communication_statistic";
const std::string OP_STATISTIC_NAME = "op_statistic";

const int INVALID_INDEX = -1;
const std::string OUTPUT_PATH = "mindstudio_profiler_output";
const std::string SUMMARY_SUFFIX = ".csv";
const std::string SLICE = "slice";
constexpr uint32_t CSV_LIMIT = 1000000; // csv文件每100w条记录切片一次
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_CONSTANT_H
