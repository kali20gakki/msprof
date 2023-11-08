/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : event_grouper.h
 * Description        : EventGrouper模块：多线程解析bin, 生成Event插入到对应类型EventQueue
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_EVENT_GROUPER_H
#define ANALYSIS_PARSER_HOST_CANN_EVENT_GROUPER_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include "event.h"
#include "event_queue.h"
#include "cann_warehouse.h"
#include "safe_unordered_map.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {

using SafeUnorderedMap = Analysis::Utils::SafeUnorderedMap<uint32_t, std::shared_ptr<CANNWarehouse>>;

class EventGrouper {
public:
    // hostPath为采集侧落盘host二进制数据路径
    explicit EventGrouper(const std::string &hostPath) : hostPath_(hostPath)
    {}
    // 多线程解析bin,生成Event插入到对应类型EventQueue
    bool Group();

    // 获取解析分类完成的Event, Key为threadId
    std::unordered_map<uint32_t, std::shared_ptr<CANNWarehouse>> &GetGroupEvents();

private:
    std::string hostPath_;
    SafeUnorderedMap cannWarehouses_; // 所有threadId的数据
};

} // namespace Cann
} // namespace Host
} // namespace Parser
} // namespace Analysis
#endif // ANALYSIS_PARSER_HOST_CANN_EVENT_GROUPER_H

