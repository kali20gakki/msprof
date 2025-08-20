/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : kernel_parser.cpp
 * Description        : 缓存处理runtime tasktrack数据
 * Author             : msprof team
 * Creation Date      : 2025/08/07
 * *****************************************************************************
 */

#ifndef MSPTI_PARSER_KERNEL_PARSER_H
#define MSPTI_PARSER_KERNEL_PARSER_H

#include <memory>
#include "external/mspti_result.h"
#include "external/mspti_activity.h"
#include "common/inject/profapi_inject.h"

namespace Mspti {
namespace Parser {
class KernelParser {
public:
    static KernelParser &GetInstance();
    msptiResult ReportRtTaskTrack(uint32_t agingFlag, const MsprofCompactInfo *data);
private:
    KernelParser();
    ~KernelParser();
    class KernelParserImpl;
    std::unique_ptr<KernelParserImpl> pImpl;
};
}
}

#endif // MSPTI_PARSER_KERNEL_PARSER_H
