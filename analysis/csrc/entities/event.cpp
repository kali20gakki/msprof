/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : event.cpp
 * Description        : Event模块：定义Event数据结构
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#include "event.h"

#include <utility>

namespace Analysis {
namespace Entities {

std::atomic<uint32_t> g_eventIDCnt{0};

Event::Event(std::shared_ptr<char> eventPtr, std::string eventDesc, Analysis::Entities::EventInfo &eventInfo)
    : event(std::move(eventPtr)), desc(std::move(eventDesc)), info(eventInfo), id(++g_eventIDCnt)
{}

} // namespace Entities
} // namespace Analysis