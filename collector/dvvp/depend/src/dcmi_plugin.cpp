/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: dcmi interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-04-22
 */
#include "dcmi_plugin.h"
#include "msprof_dlog.h"

using namespace analysis::dvvp::common::error;

namespace Collector {
namespace Dvvp {
namespace Plugin {
SHARED_PTR_ALIA<PluginHandle> DcmiPlugin::pluginHandle_ = nullptr;

void DcmiPlugin::GetAllFunction()
{
    pluginHandle_->GetFunction<int>("dcmi_init", dcmiInitFunc_);
    pluginHandle_->GetFunction<int, int *, int *, int>("dcmi_get_card_list", dcmiGetCardListFunc_);
    pluginHandle_->GetFunction<int, int, int *>("dcmi_get_device_num_in_card", dcmiGetDeviceNumInCardFunc_);
    pluginHandle_->GetFunction<int, int, int, int, struct dcmi_network_pkt_stats_info *>(
        "dcmi_get_netdev_pkt_stats_info", dcmiGetNetdevPktStatsInfoFunc_);
}

void DcmiPlugin::LoadDcmiSo()
{
    if (pluginHandle_ == nullptr) {
        MSVP_MAKE_SHARED1_VOID(pluginHandle_, PluginHandle, soName_);
    }
    int32_t ret = PROFILING_SUCCESS;
    if (!pluginHandle_->HasLoad()) {
        bool isSoValid = true;
        if (pluginHandle_->OpenPluginFromEnv("LD_LIBRARY_PATH", isSoValid) != PROFILING_SUCCESS &&
            pluginHandle_->OpenPluginFromLdcfg(isSoValid) != PROFILING_SUCCESS) {
            MSPROF_LOGE("DcmiPlugin failed to load dcmi so");
            return;
        }

        if (!isSoValid) {
            MSPROF_LOGW("Dcmi so it may not be secure because there are incorrect permissions");
        }
    }
    GetAllFunction();
}

bool DcmiPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_->IsFuncExist(funcName);
}

int32_t DcmiPlugin::MsprofDcmiInit()
{
    PthreadOnce(&loadFlag_, []()->void {DcmiPlugin::instance()->LoadDcmiSo();});
    if (dcmiInitFunc_ == nullptr) {
        MSPROF_LOGW("DcmiPlugin dcmi_init function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return dcmiInitFunc_() == DCMI_OK ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int32_t DcmiPlugin::MsprofDcmiGetCardList(int *cardNum, int *cardList, int listLen)
{
    PthreadOnce(&loadFlag_, []()->void {DcmiPlugin::instance()->LoadDcmiSo();});
    if (dcmiGetCardListFunc_ == nullptr) {
        MSPROF_LOGW("DcmiPlugin dcmi_get_card_list function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return dcmiGetCardListFunc_(cardNum, cardList, listLen) == DCMI_OK ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int32_t DcmiPlugin::MsprofDcmiGetDeviceNumInCard(int cardId, int *deviceNum)
{
    PthreadOnce(&loadFlag_, []()->void {DcmiPlugin::instance()->LoadDcmiSo();});
    if (dcmiGetDeviceNumInCardFunc_ == nullptr) {
        MSPROF_LOGW("DcmiPlugin dcmi_get_device_num_in_card function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return dcmiGetDeviceNumInCardFunc_(cardId, deviceNum) == DCMI_OK ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int32_t DcmiPlugin::MsprofDcmiGetNetdevPktStatsInfo(int cardId, int deviceId, int portId,
    struct dcmi_network_pkt_stats_info *statsInfo)
{
    PthreadOnce(&loadFlag_, []()->void {DcmiPlugin::instance()->LoadDcmiSo();});
    if (dcmiGetNetdevPktStatsInfoFunc_ == nullptr) {
        MSPROF_LOGW("DcmiPlugin dcmi_get_netdev_pkt_stats_info function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return dcmiGetNetdevPktStatsInfoFunc_(cardId, deviceId, portId, statsInfo) == DCMI_OK ?
        PROFILING_SUCCESS : PROFILING_FAILED;
}
} // namespace Plugin
} // namespace Dvvp
} // namespace Collector
