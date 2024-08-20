/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : json_sample.h
 * Description        : json dump工具测试使用的样例
 * Author             : msprof team
 * Creation Date      : 2024/8/9
 * *****************************************************************************
 */
#ifndef ANALYSIS_UT_INFRASTRUCTURE_DUMP_TOOLS_SAMPLE_TYPES_H
#define ANALYSIS_UT_INFRASTRUCTURE_DUMP_TOOLS_SAMPLE_TYPES_H

#include <tuple>
#include <string>
#include <vector>

// 如下的例子，是Json嵌套的情况，定义的结构中， DumpToolSampleStruct 中嵌套了 NestStructure
using NestStructTp = std::tuple<std::string>;
struct NestStructure {
    std::vector<std::string> columns;
    std::vector<NestStructTp> data;
};

using DumpToolSampleTp = std::tuple<uint64_t, int64_t, std::string, double, float, NestStructure, int, bool>;
struct DumpToolSampleStruct {
    std::vector<std::string> columns;
    std::vector<DumpToolSampleTp> data;
};

#endif