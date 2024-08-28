/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : json_constant.h
 * Description        : json交付件相关的常量
 * Author             : msprof team
 * Creation Date      : 2024/8/24
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_DELIVERABLES_CONSTANT_H
#define ANALYSIS_APPLICATION_DELIVERABLES_CONSTANT_H

#include <string>
#include <cstdint>

namespace Analysis {
namespace Application {
const uint8_t ASSEMBLE_FAILED = 0;
const uint8_t ASSEMBLE_SUCCESS = 1;
const uint8_t DATA_NOT_EXIST = 2;
const int DEFAULT_TID = 0;
const uint32_t HOST_PID = 31;
const int HIGH_BIT_OFFSET = 10;
const int MIDDLE_BIT_OFFSET = 5;
const std::string META_DATA_PROCESS_NAME = "process_name";
const std::string META_DATA_PROCESS_INDEX = "process_sort_index";
const std::string META_DATA_PROCESS_LABEL = "process_labels";
const std::string META_DATA_THREAD_NAME = "thread_name";
const std::string META_DATA_THREAD_INDEX = "thread_sort_index";
const std::string HOST_TO_DEVICE = "HostToDevice";
const std::string MS_TX = "MsTx_";
const std::string FLOW_START = "S";
const std::string FLOW_END = "F";
const std::string FLOW_BP = "E";
const std::string OUTPUT_PATH = "mindstudio_profiler_output";
const std::string JSON_SUFFIX = ".json";
const double NS_TO_MS = 1000.0;
const int CONN_OFFSET = 32;
/*
 * json格式要求多个对象使用[]包装，再每一层json后添加了","分割，最终会形成[{},true]的结果，因此需要写入内容的时候过滤掉
 * [ true],共6位长度
 */
const std::size_t FILE_CONTENT_SUFFIX = 6;
}
}
#endif // ANALYSIS_APPLICATION_DELIVERABLES_CONSTANT_H
