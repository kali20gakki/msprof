/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : trace_event.cpp
 * Description        : json落盘相关的trace对象
 * Author             : msprof team
 * Creation Date      : 2024/8/21
 * *****************************************************************************
 */

#include "analysis/csrc/domain/entities/json_trace/include/trace_event.h"

namespace Analysis {
namespace Domain {
TraceEvent::TraceEvent(uint32_t pid, int tid, const std::string &name) : pid_(pid), tid_(tid), name_(name) {}

void TraceEvent::ToJson(JsonWriter &ostream)
{
    ostream["name"] << name_;
    ostream["pid"] << pid_;
    ostream["tid"] << tid_;
}

DurationEvent::DurationEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name)
    : TraceEvent(pid, tid, name), dur_(dur), ts_(ts) {}
void DurationEvent::ToJson(JsonWriter &ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ts"] << ts_;
    ostream["dur"] << dur_;
    ostream["ph"] << ph_;
    ostream["args"];
    // 嵌套json，需要再包一层{}，其中ProcessArgs函数为子类实现，子类只需要关注args参数部分实现，无需感知其他参数
    ostream.StartObject();
    ProcessArgs(ostream);
    ostream.EndObject();
}

CounterEvent::CounterEvent(uint32_t pid, int tid, const std::string &ts, const std::string &name)
    : TraceEvent(pid, tid, name), ts_(ts) {}
void CounterEvent::ToJson(JsonWriter &ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ts"] << ts_;
    ostream["ph"] << ph_;
    ostream["args"];
    ostream.StartObject();
    ProcessArgs(ostream);
    ostream.EndObject();
}

void CounterEvent::ProcessArgs(JsonWriter &ostream)
{
    for (const auto &kv: seriesValue_) {
        ostream[kv.first.c_str()] << kv.second;
    }
}

void CounterEvent::SetSeriesValue(const std::string &key, const uint64_t &value)
{
    seriesValue_[key] = value;
}

FlowEvent::FlowEvent(uint32_t pid, int tid, const std::string &ts, const std::string &cat, const std::string &id,
                     const std::string &name, const std::string &ph)
    : TraceEvent(pid, tid, name), ts_(ts), cat_(cat), id_(id), ph_(ph), bp_(" ") {}
FlowEvent::FlowEvent(uint32_t pid, int tid, const std::string &ts, const std::string &cat, const std::string &id,
                     const std::string &name, const std::string &ph, const std::string &bp)
    : TraceEvent(pid, tid, name), ts_(ts), cat_(cat), id_(id), ph_(ph), bp_(bp) {}
void FlowEvent::ToJson(JsonWriter &ostream)
{
    TraceEvent::ToJson(ostream);
    ostream["ph"] << ph_;
    ostream["cat"] << cat_;
    ostream["id"] << id_;
    ostream["ts"] << ts_;
    if (bp_ != " ") {
        ostream["bp"] << bp_;
    }
}
}
}