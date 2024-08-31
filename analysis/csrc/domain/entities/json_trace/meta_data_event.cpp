/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : meta_data_event.cpp
 * Description        : metadata落盘相关的trace对象
 * Author             : msprof team
 * Creation Date      : 2024/8/21
 * *****************************************************************************
 */

#include "analysis/csrc/domain/entities/json_trace/include/meta_data_event.h"
#include <unordered_map>
#include "analysis/csrc/utils/utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Application;
using namespace Analysis::Utils;

MetaDataEvent::MetaDataEvent(uint32_t pid, int tid, const std::string &name) : TraceEvent(pid, tid, name) {}

void MetaDataEvent::ToJson(JsonWriter &ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ph"] << ph_;
    ostream["args"];
    // 嵌套json，需要再包一层{}，其中ProcessArgs函数为子类实现，子类只需要关注args参数部分实现，无需感知其他参数
    ostream.StartObject();
    ProcessArgs(ostream);
    ostream.EndObject();
}

MetaDataNameEvent::MetaDataNameEvent(uint32_t pid, int tid, const std::string &name, const std::string &argName)
    : MetaDataEvent(pid, tid, name), argsName_(argName) {}

void MetaDataNameEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["name"] << argsName_;
}

MetaDataLabelEvent::MetaDataLabelEvent(uint32_t pid, int tid, const std::string &name, const std::string &label)
    : MetaDataEvent(pid, tid, name), argsLabel_(label) {}

void MetaDataLabelEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["labels"] << argsLabel_;
}

MetaDataIndexEvent::MetaDataIndexEvent(uint32_t pid, int tid, const std::string &name, int index)
    : MetaDataEvent(pid, tid, name), argsSortIndex_(index) {}

void MetaDataIndexEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["sort_index"] << argsSortIndex_;
}

LayerInfo GetLayerInfo(std::string processName)
{
    return LAYER_INFO.at(processName);
}

void GenerateMetaData(const std::unordered_map<uint16_t, uint32_t> &pidMap, const struct LayerInfo &layerInfo,
                      std::vector<std::shared_ptr<TraceEvent>> &res)
{
    for (const auto kv: pidMap) {
        std::shared_ptr<MetaDataNameEvent> processName;
        MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, kv.second, DEFAULT_TID,
                                META_DATA_PROCESS_NAME, layerInfo.component);
        std::shared_ptr<MetaDataLabelEvent> processLabel;
        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, kv.second, DEFAULT_TID,
                                META_DATA_PROCESS_LABEL, layerInfo.label);
        std::shared_ptr<MetaDataIndexEvent> processIndex;
        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, kv.second, DEFAULT_TID,
                                META_DATA_PROCESS_INDEX, layerInfo.sortIndex);
        res.push_back(processName);
        res.push_back(processLabel);
        res.push_back(processIndex);
    }
}
}
}