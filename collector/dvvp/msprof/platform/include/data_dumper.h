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
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_ENGINE_DATA_DUMPER_H
#define MSPROF_ENGINE_DATA_DUMPER_H

#include <string>
#include "utils.h"
#include "prof_reporter.h"
#include "thread/thread.h"
#include "receive_data.h"

namespace Msprof {
namespace Engine {
class DataDumper : public ReceiveData, public ProfReporter, public analysis::dvvp::common::thread::Thread {
public:
    /* *
     * @brief DataDumper: the construct function
     */
    explicit DataDumper();
    virtual ~DataDumper();
};
} // namespace Engine
} // namespace Msprof
#endif
