/**
* @file ctrl_files_dumper.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
        * but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "ctrl_files_dumper.h"

#include "utils/utils.h"
#include "transport/transport.h"
#include "mmpa_api.h"
#include "transport/uploader_mgr.h"

#include "message/data_define.h"
#include "config/config.h"

namespace analysis {
namespace dvvp {
namespace transport {

namespace {
static const int TIME_US = 1000000;
}

using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

CtrlFilesDumper::CtrlFilesDumper() {}
CtrlFilesDumper::~CtrlFilesDumper() {}

int CtrlFilesDumper::DumpCollectionTimeInfo(uint32_t devId, bool isHostProfiling, bool isStartTime)
{
    std::string devIdStr = std::to_string(devId);
    auto collectionTime = GetHostTime();
    MSPROF_LOGI("[DumpCollectionTimeInfo]collectionTime:%s us, isStartTime:%d", collectionTime.c_str(), isStartTime);
    // time to unix
    analysis::dvvp::message::CollectionStartEndTime timeInfo;
    if (!isStartTime) {
        timeInfo.collectionTimeEnd = collectionTime;
        timeInfo.collectionDateEnd = Utils::TimestampToTime(collectionTime, TIME_US);
    } else {
        timeInfo.collectionTimeBegin = collectionTime;
        timeInfo.collectionDateBegin = Utils::TimestampToTime(collectionTime, TIME_US);
    }
    timeInfo.clockMonotonicRaw = std::to_string(Utils::GetClockMonotonicRaw());
    nlohmann::json root;
    timeInfo.ToObject(root);
    std::string content = root.dump();
    MSPROF_LOGI("[DumpCollectionTimeInfo]content:%s", content.c_str());
    SHARED_PTR_ALIA <analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->job_id = devIdStr;
    std::string fileName;
    GenerateCollectionTimeInfoName(fileName, devIdStr, isHostProfiling, isStartTime);
    analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
                                                             FileChunkDataModule::PROFILING_IS_CTRL_DATA);
    MSPROF_LOGI("[DumpCollectionTimeInfo]job_id: %s,fileName: %s", devIdStr.c_str(), fileName.c_str());
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadCtrlFileData(devIdStr, content,
                                                                                     fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", fileName.c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to upload data for %s", fileName.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

std::string CtrlFilesDumper::GetHostTime()
{
    std::string hostTime;
    MmTimeval tv;

    if (memset_s(&tv, sizeof(tv), 0, sizeof(tv)) != EOK) {
        MSPROF_LOGE("memset failed");
        return "";
    }
    if (MmGetTimeOfDay(&tv, nullptr) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetHostTime]gettimeofday failed");
        MSPROF_INNER_ERROR("EK9999", "gettimeofday failed");
    } else {
        hostTime = std::to_string((unsigned long long) tv.tvSec * TIME_US + (unsigned long long) tv.tvUsec);
    }
    return hostTime;
}

void CtrlFilesDumper::GenerateCollectionTimeInfoName(std::string& filename, const std::string& devIdStr,
                                                     bool isHostProfiling, bool isStartTime)
{
    std::string filePrefix = (isStartTime) ? "start_info" : "end_info";
    filename.append(filePrefix);
    if (!isHostProfiling) {
        filename.append(".").append(devIdStr);
    }
}

} // transport
} // dvvp
} // analysis
