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
