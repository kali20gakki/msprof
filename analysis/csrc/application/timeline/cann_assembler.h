/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_assembler.h
 * Description        : 组合CANN层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/24
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_CANN_ASSEMBLER_H
#define ANALYSIS_APPLICATION_CANN_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis {
namespace Application {
class CannAssembler : public JsonAssembler {
public:
    CannAssembler();
private:
    bool FlushToFile(JsonWriter &ostream, const std::string &profPath) override;
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
};

class ApiTraceEvent : public DurationEvent {
public:
    ApiTraceEvent(int pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t threadId,
                  uint64_t connectionId, const std::string &mode, const std::string level, const std::string id,
                  const std::string itemId)
        : DurationEvent(pid, tid, dur, ts, name), threadId_(threadId), connectionId_(connectionId), mode_(mode),
        level_(level), id_(id), itemId_(itemId) {}
private:
    void ProcessArgs(JsonWriter &ostream) override
    {
        ostream["Thread Id"] << threadId_;
        ostream["Mode"] << mode_;
        ostream["level"] << level_;
        ostream["id"] << id_;
        ostream["item_id"] << itemId_;
        ostream["connection_id"] << connectionId_;
    }
private:
    uint32_t threadId_;
    uint64_t connectionId_;
    std::string mode_;
    std::string level_;
    std::string id_;
    std::string itemId_;
};
}
}
#endif // ANALYSIS_APPLICATION_CANN_ASSEMBLER_H
