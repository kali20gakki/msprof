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
#ifndef ANALYSIS_UT_INFRASTRUCTURE_DUMP_TOOLS_JSON_SAMPLE_H
#define ANALYSIS_UT_INFRASTRUCTURE_DUMP_TOOLS_JSON_SAMPLE_H

#include "sample/sample_types.h"
#include "analysis/csrc/infrastructure/dump_tools/json_tool/include/json_writer.h"

Analysis::Infra::JsonWriter& operator<<(Analysis::Infra::JsonWriter& stream, const DumpToolSampleStruct& sample);

#endif