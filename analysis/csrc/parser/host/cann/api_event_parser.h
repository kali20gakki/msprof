/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_event_parser.h
 * Description        : api event数据解析
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_API_EVENT_PARSER_H
#define ANALYSIS_PARSER_HOST_CANN_API_EVENT_PARSER_H

#include <string>

#include "base_parser.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {
// 该类的作用是Api和Event数据的解析
class ApiEventParser final : public BaseParser {
public:
    explicit ApiEventParser(const std::string &path);

private:
    int ProduceChunk() override;
    int ConsumeChunk(std::shared_ptr<void> &chunk, const std::shared_ptr<ChunkGenerator> &chunkConsumer) override;

private:
    std::vector<std::string> filePrefix_ = {
        "unaging.api_event.data.slice",
        "aging.api_event.data.slice",
    };
};  // class ApiEventParser
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis
#endif // ANALYSIS_PARSER_HOST_CANN_API_EVENT_PARSER_H
