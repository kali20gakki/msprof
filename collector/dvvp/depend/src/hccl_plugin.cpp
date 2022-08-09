/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: driver interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-08
 */
#include "hccl_plugin.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
void HcclPlugin::LoadDriverSo()
{
    if (pluginHandle_.HasLoad()) {
        return;
    }
    int32_t ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
    if (ret != PROFILING_SUCCESS) {
        return;
    }    
}

bool HcclPlugin::IsFuncExist(const std::string &funcName)
{
    PthreadOnce(&loadFlag_, []()->void {HcclPlugin::instance()->LoadDriverSo();});
    return pluginHandle_.IsFuncExist(funcName);
}

// HcomGetRankId
int32_t HcclPlugin::MsprofHcomGetRankId(uint32_t *rankId)
{
    PthreadOnce(&loadFlag_, []()->void {HcclPlugin::instance()->LoadDriverSo();});
    if (hcomGetRankId_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<uint32_t, const char *, uint32_t *>("HcomGetRankId",
            hcomGetRankId_);
        if (ret != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
    return hcomGetRankId_(nullptr, rankId);
}
} // Plugin
} // Dvvp
} // Collector