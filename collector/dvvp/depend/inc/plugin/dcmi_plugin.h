/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: dcmi interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-04-22
 */
#ifndef DCMI_PLUGIN_H
#define DCMI_PLUGIN_H

#include "singleton/singleton.h"
#include "dcmi/dcmi_api.h"
#include "plugin_handle.h"
#include "utils.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using DcmiInitFunc = std::function<int()>;
using DcmiGetCardListFunc = std::function<int(int *, int *, int)>;
using DcmiGetDeviceNumInCardFunc = std::function<int(int, int *)>;
using DcmiGetNetdevPktStatsInfoFunc = std::function<int(int, int, int, struct dcmi_network_pkt_stats_info *)>;

class DcmiPlugin : public analysis::dvvp::common::singleton::Singleton<DcmiPlugin> {
public:
    DcmiPlugin() : soName_("libdcmi.so"), loadFlag_(0) {}

    bool IsFuncExist(const std::string &funcName) const;

    // dcmi_init
    int MsprofDcmiInit();

    // dcmi_get_card_list
    int MsprofDcmiGetCardList(int *cardNum, int *cardList, int listLen);

    // dcmi_get_device_num_in_card
    int MsprofDcmiGetDeviceNumInCard(int cardId, int *deviceNum);

    // dcmi_get_netdev_pkt_stats_info
    int MsprofDcmiGetNetdevPktStatsInfo(int cardId, int deviceId, int portId,
        struct dcmi_network_pkt_stats_info *statsInfo);

private:
    std::string soName_;
    static SHARED_PTR_ALIA<PluginHandle> pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    DcmiInitFunc dcmiInitFunc_ = nullptr;
    DcmiGetCardListFunc dcmiGetCardListFunc_ = nullptr;
    DcmiGetDeviceNumInCardFunc dcmiGetDeviceNumInCardFunc_ = nullptr;
    DcmiGetNetdevPktStatsInfoFunc dcmiGetNetdevPktStatsInfoFunc_ = nullptr;

private:
    void LoadDcmiSo();

    // get all function addresses at a time
    void GetAllFunction();
};

} // namespace Plugin
} // namespace Dvvp
} // namespace Collector
#endif // DCMI_PLUGIN_H
