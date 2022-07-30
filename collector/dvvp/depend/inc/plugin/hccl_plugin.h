/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: driver interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#ifndef DRIVER_PLUGIN_H
#define DRIVER_PLUGIN_H

#include "singleton/singleton.h"
// #include "driver/ascend_hal.h"
// #include "hccl/hcom.h"
#include "plugin_handle.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using HcomGetLocalRankIdFunc = std::function<uint32_t(const char *group, uint32_t *localRankId)>;
class HcclPlugin : public analysis::dvvp::common::singleton::Singleton<DriverPlugin> {
public:
    HcclPlugin()
     :soName_("libhccl.so"),
      pluginHandle_(PluginHandle(soName_)),
      loadFlag_(0)
    {}

    bool IsFuncExist(const std::string &funcName) const;

    // HcomGetLocalRankId
    uint32_t MsprofHcomGetLocalRankId(uint32_t *localRankId);

private:
    std::string soName_;
    PluginHandle pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    HcomGetLocalRankIdFunc hcomGetLocalRankId_ = nullptr;
 private:
    void LoadDriverSo();
};

} // Plugin
} // Dvvp
} // Collector

#endif