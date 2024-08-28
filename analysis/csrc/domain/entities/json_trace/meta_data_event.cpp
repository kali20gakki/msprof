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

namespace Analysis {
namespace Domain {
MetaDataEvent::MetaDataEvent(int pid, int tid, const std::string &name) : TraceEvent(pid, tid, name) {}

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

MetaDataNameEvent::MetaDataNameEvent(int pid, int tid, const std::string &name, const std::string &argName)
    : MetaDataEvent(pid, tid, name), argsName_(argName) {}

void MetaDataNameEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["name"] << argsName_;
}

MetaDataLabelEvent::MetaDataLabelEvent(int pid, int tid, const std::string &name, const std::string &label)
    : MetaDataEvent(pid, tid, name), argsLabel_(label) {}

void MetaDataLabelEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["labels"] << argsLabel_;
}

MetaDataIndexEvent::MetaDataIndexEvent(int pid, int tid, const std::string &name, int index)
    : MetaDataEvent(pid, tid, name), argsSortIndex_(index) {}

void MetaDataIndexEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["sort_index"] << argsSortIndex_;
}
}
}