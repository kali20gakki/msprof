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
#ifndef ANALYSIS_DVPP_TRANSPORT_CTRL_FILES_DUMPER_H
#define ANALYSIS_DVPP_TRANSPORT_CTRL_FILES_DUMPER_H

#include <cstring>

#include "singleton/singleton.h"
#include "message/prof_params.h"

namespace analysis {
namespace dvvp {
namespace transport {

class CtrlFilesDumper : public analysis::dvvp::common::singleton::Singleton<CtrlFilesDumper> {
public:
    CtrlFilesDumper();
    virtual ~CtrlFilesDumper();

    int DumpCollectionTimeInfo(uint32_t devId, bool isHostProfiling, bool isStartTime);

private:
    void GenerateCollectionTimeInfoName(std::string& filename, const std::string& deviceId,
                                        bool isHostProfiling, bool isStartTime);

    std::string GetHostTime();
};

} // transport
} // dvvp
} // analysis

#endif // ANALYSIS_DVPP_TRANSPORT_CTRL_FILES_DUMPER_H
