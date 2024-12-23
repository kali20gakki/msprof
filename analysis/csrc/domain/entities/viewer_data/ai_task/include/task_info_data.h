/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_info_data.h
 * Description        : taskInfo表format后的数据格式
 * Author             : msprof team
 * Creation Date      : 2024/8/1
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_TASK_INFO_DATA_H
#define ANALYSIS_DOMAIN_TASK_INFO_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
const uint8_t DEFAULT_MULTIPLE_SIZE = 2;
const std::string SINGLE_OPERATOR = "\"";
const std::string CSV_OPERATOR = R"(""")";

struct ShapeInfo {
    std::string inputFormats;
    std::string inputDataTypes;
    std::string inputShapes;
    std::string outputFormats;
    std::string outputDataTypes;
    std::string outputShapes;
};

struct TaskInfoData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t blockDim = UINT32_MAX;
    uint32_t mixBlockDim = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    std::string opState;
    std::string hashId;
    std::string opName;
    std::string taskType;
    std::string opType;
    std::string opFlag;
    std::string inputFormats;
    std::string inputDataTypes;
    std::string inputShapes;
    std::string outputFormats;
    std::string outputDataTypes;
    std::string outputShapes;

    TaskInfoData& operator=(const ShapeInfo &shapeInfo)
    {
        inputFormats = escapeQuotes(shapeInfo.inputFormats);
        inputDataTypes = escapeQuotes(shapeInfo.inputDataTypes);
        inputShapes = escapeQuotes(shapeInfo.inputShapes);
        outputFormats = escapeQuotes(shapeInfo.outputFormats);
        outputDataTypes = escapeQuotes(shapeInfo.outputDataTypes);
        outputShapes = escapeQuotes(shapeInfo.outputShapes);
        return *this;
    }

private:
    std::string escapeQuotes(const std::string &input)
    {
        std::string res = input;
        res.reserve(input.size() * DEFAULT_MULTIPLE_SIZE);
        std::string::size_type pos = 0;
        while ((pos = res.find(SINGLE_OPERATOR, pos)) != std::string::npos) {
            res.replace(pos, SINGLE_OPERATOR.length(), CSV_OPERATOR);
            pos += CSV_OPERATOR.length();
        }
        return res;
    }
};
}
}
#endif // ANALYSIS_DOMAIN_TASK_INFO_DATA_H
