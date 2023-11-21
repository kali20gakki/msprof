/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : addition_info_parser.h
 * Description        : addition数据解析
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_ADDITION_INFO_PARSER_H
#define ANALYSIS_PARSER_HOST_CANN_ADDITION_INFO_PARSER_H

#include <string>
#include <vector>

#include "base_parser.h"
#include "prof_common.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {
// 该类的作用是Addition数据的解析
class AdditionInfoParser : public BaseParser {
public:
    explicit AdditionInfoParser(const std::string &path) : BaseParser(path) {}
    void Init(const std::vector<std::string> &filePrefix);

private:
    int ConsumeChunk(std::shared_ptr<void> &chunk, const std::shared_ptr<ChunkGenerator> &chunkConsumer) override;
};  // class AdditionInfoParser

// 该类的作用是CtxId数据的解析
class CtxIdParser final : public AdditionInfoParser {
public:
    explicit CtxIdParser(const std::string &path) : AdditionInfoParser(path)
    {
        Init(filePrefix_);
    }

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.context_id_info.slice",
        "aging.additional.context_id_info.slice",
    };
};  // class CtxIdParser

// 该类的作用是fusion op数据的解析
class FusionOpInfoParser final : public AdditionInfoParser {
public:
    explicit FusionOpInfoParser(const std::string &path) : AdditionInfoParser(path)
    {
        Init(filePrefix_);
    }

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.fusion_op_info.slice",
        "aging.additional.fusion_op_info.slice",
    };
};  // class FusionOpInfoParser

// 该类的作用是fusion op数据的解析
class GraphIdParser final : public AdditionInfoParser {
public:
    explicit GraphIdParser(const std::string &path) : AdditionInfoParser(path)
    {
        Init(filePrefix_);
    }

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.graph_id_map.slice",
        "aging.additional.graph_id_map.slice",
    };
};  // class GraphIdParser

// 该类的作用是tensor info数据的解析
class TensorInfoParser final : public AdditionInfoParser {
public:
    explicit TensorInfoParser(const std::string &path);

private:
    int ProduceChunk() override;
    static std::shared_ptr<ConcatTensorInfo> CreateConcatTensorInfo(
        const std::shared_ptr<MsprofAdditionalInfo> &additionalInfo);

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.tensor_info.slice",
        "aging.additional.tensor_info.slice",
    };
};  // class TensorInfoParser

// 该类的作用是hccl info数据的解析
class HcclInfoParser final : public AdditionInfoParser {
public:
    explicit HcclInfoParser(const std::string &path) : AdditionInfoParser(path)
    {
        Init(filePrefix_);
    }

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.hccl_info.slice",
        "aging.additional.hccl_info.slice",
    };
};  // class HcclInfoParser

// 该类的作用是memory application数据的解析
class MemoryApplicationParser final : public AdditionInfoParser {
public:
    explicit MemoryApplicationParser(const std::string &path) : AdditionInfoParser(path)
    {
        Init(filePrefix_);
    }

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.memory_application.slice",
        "aging.additional.memory_application.slice",
    };
};  // class MemoryApplicationParser

// 该类的作用是multi thread数据的解析
class MultiThreadParser final : public AdditionInfoParser {
public:
    explicit MultiThreadParser(const std::string &path) : AdditionInfoParser(path)
    {
        Init(filePrefix_);
    }

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.Multi_Thread.slice",
        "aging.additional.Multi_Thread.slice",
    };
};  // class MultiThreadParser
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis

#endif // ANALYSIS_PARSER_HOST_CANN_ADDITION_INFO_PARSER_H
