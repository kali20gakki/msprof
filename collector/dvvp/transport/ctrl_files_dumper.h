/**
* @file ctrl_files_dumper.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
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
