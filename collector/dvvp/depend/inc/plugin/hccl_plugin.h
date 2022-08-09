/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: hccl interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-08
 */
#ifndef HCCL_PLUGIN_H
#define HCCL_PLUGIN_H

#include "singleton/singleton.h"
#include "plugin_handle.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using HcomGetRankIdFunc = std::function<uint32_t(const char *group, uint32_t *rankId)>;
class HcclPlugin : public analysis::dvvp::common::singleton::Singleton<HcclPlugin> {
public:
    HcclPlugin() : soName_("libhccl.so"), pluginHandle_(PluginHandle(soName_)), loadFlag_(0) {}
    bool IsFuncExist(const std::string &funcName) const;
    // HcomGetRankId
    int32_t MsprofHcomGetRankId(uint32_t *rankId);

private:
    std::string soName_;
    PluginHandle pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    HcomGetRankIdFunc hcomGetRankId_ = nullptr;

 private:
    void LoadDriverSo();
};
} // Plugin
} // Dvvp
} // Collector

#endif