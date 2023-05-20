/**
 * @file analyzer_rt.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include "analyzer_rt.h"
#include "acl_prof.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "proto/profiler.pb.h"
#include "prof_common.h"
#include "prof_api.h"
 
namespace Analysis {
namespace Dvvp {
namespace Analyze {
static const uint32_t RT_COMPACT_INFO_SIZE = sizeof(MsprofCompactInfo);
 
bool AnalyzerRt::IsRtCompactData(const std::string &tag) const
{
    if (tag.find("task_track") != std::string::npos) {
        return true;
    }
 
    return false;
}

/*
 * @berif  : parse runtime task_track data
 * @param  : None
 * @return : None
 */
void AnalyzerRt::RtCompactParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        return;
    }
 
    totalBytes_ += message->chunksizeinbytes();
    if (message->filename().find("unaging") != std::string::npos) {
        ParseRuntimeTrackData(message->chunk().c_str(), message->chunksizeinbytes(), false);
        return;
    }
}

void AnalyzerRt::ParseRuntimeTrackData(CONST_CHAR_PTR data, uint32_t len, bool ageFlag)
{
}
 
void AnalyzerRt::HandleRuntimeTrackData(CONST_CHAR_PTR data, bool ageFlag)
{
}
 
void AnalyzerRt::PrintStats()
{
    MSPROF_EVENT("total_size_analyze, module: RT, analyzed %llu, total %llu, rt time %u, merge %u",
        analyzedBytes_, totalBytes_, totalRtTimes_, totalRtMerges_);
}
 
}
}
}