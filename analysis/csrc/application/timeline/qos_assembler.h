/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : qos_assembler.h
 * Description        : 组合QoS层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_QOS_ASSEMBLER_H
#define ANALYSIS_APPLICATION_QOS_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis {
namespace Application {
class QosAssembler : public JsonAssembler {
public:
    QosAssembler();
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
};
}
}
#endif // ANALYSIS_APPLICATION_QOS_ASSEMBLER_H
