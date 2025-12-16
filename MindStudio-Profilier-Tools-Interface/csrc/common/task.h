/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#ifndef MSPTI_COMMON_TASK_H
#define MSPTI_COMMON_TASK_H

#include "csrc/common/bound_queue.h"
#include "csrc/include/mspti_result.h"

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
