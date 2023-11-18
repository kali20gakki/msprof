/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_warehouse.h
 * Description        : CANNWareHouse模块：包含按照EventLevel分好类的Event数据
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_CANN_WAREHOUSE_H
#define ANALYSIS_PARSER_HOST_CANN_CANN_WAREHOUSE_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include "event.h"
#include "event_queue.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {

using EventQueue = Analysis::Entities::EventQueue;

// CANN数据仓，用于存储各个Type的Events
struct CANNWarehouse {
    // API Type只在Model、Node、HCCL Level的Event
    std::shared_ptr<EventQueue> kernelEvents;
    std::shared_ptr<EventQueue> graphIdMapEvents;
    std::shared_ptr<EventQueue> fusionOpInfoEvents;
    std::shared_ptr<EventQueue> nodeBasicInfoEvents;
    std::shared_ptr<EventQueue> tensorInfoEvents;
    std::shared_ptr<EventQueue> contextIdEvents;
    std::shared_ptr<EventQueue> hcclInfoEvents;
    std::shared_ptr<EventQueue> taskTrackEvents;
};

} // namespace Cann
} // namespace Host
} // namespace Parser
} // namespace Analysis
#endif // ANALYSIS_PARSER_HOST_CANN_CANN_WAREHOUSE_H
