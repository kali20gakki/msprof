/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: driver interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "hccl_plugin.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
void HcclPlugin::LoadDriverSo()
{
    int32_t ret = PROFILING_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PROFILING_SUCCESS) {
            return;
        }
    }
}

bool HcclPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_.IsFuncExist(funcName);
}

// HcomGetLocalRankId
int32_t HcclPlugin::MsprofHcomGetLocalRankId(uint32_t *localRankId)
{
    PthreadOnce(&loadFlag_, []()->void {HcclPlugin::instance()->LoadDriverSo();});
    if (hcomGetLocalRankId_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<uint32_t, const char *, uint32_t *>("HcomGetLocalRankId", hcomGetLocalRankId_);
        if (ret != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
    return hcomGetLocalRankId_(nullptr, localRankId);
}
} // Plugin
} // Dvvp
} // Collector