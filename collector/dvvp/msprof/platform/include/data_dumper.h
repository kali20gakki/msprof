/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: dump stream file
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2019-12-20
 */

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
