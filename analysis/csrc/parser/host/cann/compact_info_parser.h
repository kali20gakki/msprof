/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : compact_info_parser.h
 * Description        : compact数据解析
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_COMPACT_INFO_PARSER_H
#define ANALYSIS_PARSER_HOST_CANN_COMPACT_INFO_PARSER_H

#include <string>
#include <vector>

#include "base_parser.h"
#include "prof_common.h"
#include "flip.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {

// 该类的作用是Compact数据的解析
class CompactInfoParser : public BaseParser<CompactInfoParser> {
public:
    explicit CompactInfoParser(const std::string &path, const std::string &parserName)
        : BaseParser(path, parserName) {}
    void Init(const std::vector<std::string> &filePrefix);
    template<typename T> std::vector<std::shared_ptr<T>> GetData();

protected:
    int ProduceData() override;

protected:
    std::vector<std::shared_ptr<MsprofCompactInfo>> compactData_;  // not owned
    std::vector<std::shared_ptr<Adapter::FlipTask>> flipTaskData_;  // not owned
};  // class CompactInfoParser

// 该类的作用是node basic info数据的解析
class NodeBasicInfoParser final : public CompactInfoParser {
public:
    explicit NodeBasicInfoParser(const std::string &path) : CompactInfoParser(path, "NodeBasicInfoParser")
    {
        Init(filePrefix_);
    }

private:
    int ProduceData() override;

private:
    enum class OpType : uint8_t {
        STATIC_OP = 0,
        DYNAMIC_OP = 1,
    };
    std::vector<std::string> filePrefix_ = {
        "aging.compact.node_basic_info.slice",
    };
    std::vector<std::string> staticFilePrefix_ = {
        "unaging.compact.node_basic_info.slice",
    };
};  // class NodeBasicInfoParser

// 该类的作用是memcpy info数据的解析
class MemcpyInfoParser final : public CompactInfoParser {
public:
    explicit MemcpyInfoParser(const std::string &path) : CompactInfoParser(path, "MemcpyInfoParser")
    {
        Init(filePrefix_);
    }

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.compact.memcpy_info.slice",
        "aging.compact.memcpy_info.slice",
    };
};  // class MemcpyInfoParser

// 该类的作用是task track数据的解析
class TaskTrackParser final : public CompactInfoParser {
public:
    explicit TaskTrackParser(const std::string &path) : CompactInfoParser(path, "TaskTrackParser")
    {
        Init(filePrefix_);
    }

private:
    int ProduceData() override;

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.compact.task_track.slice",
        "aging.compact.task_track.slice",
    };
};  // class TaskTrackParser
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis

#endif // ANALYSIS_PARSER_HOST_CANN_COMPACT_INFO_PARSER_H
