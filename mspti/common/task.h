/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task.h
 * Description        : ThreadPool Task.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_TASK_H
#define MSPTI_COMMON_TASK_H

#include "common/bound_queue.h"
#include "external/mspti_result.h"

namespace Mspti {
namespace Common {

class Task {
public:
    virtual ~Task() = default;

public:
    virtual msptiResult Execute() = 0;
    virtual size_t HashId() = 0;
};

using TaskQueue = BoundQueue<std::shared_ptr<Task>>;

}  // Common
}  // Mspti

#endif
