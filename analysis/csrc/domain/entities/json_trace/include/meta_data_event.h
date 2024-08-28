/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : meta_data_event.h
 * Description        : metadata落盘相关的trace对象
 * Author             : msprof team
 * Creation Date      : 2024/8/21
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_METADATA_EVENT_H
#define MSPROF_ANALYSIS_METADATA_EVENT_H

#include "analysis/csrc/domain/entities/json_trace/include/trace_event.h"

namespace Analysis {
namespace Domain {
/**
 * MetaDataEvent对应chrome:tracing的元数据相位(MetaData Phase)，在事件跟踪中，每个事件都有一个与之相关的相位（Phase），
 * 这些相位提供了关于事件的不同方面的信息。常见的相位包括：
 *      B（Begin）：事件的开始。
 *      E（End）：事件的结束。
 *      X：完成（Complete），表示事件已经完成。
 *      M（MetaData）：提供事件的元数据信息。
 * MetaData Phase主要用于提供事件的额外信息，这些信息对于理解事件的上下文和性质非常重要。例如，它可能包含事件的类型、来源、
 * 相关参数等。通过查看MetaData Phase，可以获得关于事件的更多背景和详细信息，从而更深入地分析事件的行为和影响‌
 */
class MetaDataEvent : public TraceEvent {
public:
    MetaDataEvent(int pid, int tid, const std::string &name);
private:
    void ToJson(JsonWriter &ostream) override;
    virtual void ProcessArgs(JsonWriter &ostream) {};
private:
    std::string ph_ = "M";
};

// args为name参数的MetaData
class MetaDataNameEvent : public MetaDataEvent {
public:
    MetaDataNameEvent(int pid, int tid, const std::string &name, const std::string &argName);
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    std::string argsName_;
};

// args为labels参数的MetaData
class MetaDataLabelEvent : public MetaDataEvent {
public:
    MetaDataLabelEvent(int pid, int tid, const std::string &name, const std::string &label);
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    std::string argsLabel_;
};

// args为labels参数的MetaData
class MetaDataIndexEvent : public MetaDataEvent {
public:
    MetaDataIndexEvent(int pid, int tid, const std::string &name, int index);
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    int argsSortIndex_;
};
}
}
#endif // MSPROF_ANALYSIS_METADATA_EVENT_H
