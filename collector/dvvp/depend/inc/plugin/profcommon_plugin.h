/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: dcmi interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-04-22
 */
#ifndef PROF_COMMON_PLUGIN_H
#define PROF_COMMON_PLUGIN_H

#include "singleton/singleton.h"
#include "mstx_data_type.h"
#include "plugin_handle.h"
#include "utils.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
typedef int(*MstxInitInjectionFunc)(MstxGetModuleFuncTableFunc);
using ProfRegisteMstxFunc = std::function<void(MstxInitInjectionFunc, ProfModule)>;
using EnableMstxFunc = std::function<void(ProfModule)>;

class ProfCommonPlugin : public analysis::dvvp::common::singleton::Singleton<ProfCommonPlugin> {
public:
    ProfCommonPlugin() : soName_("libprof_common.so"), loadFlag_(0) {}

    bool IsFuncExist(const std::string &funcName) const;

    void MsprofProfRegisteMstxFunc(MstxInitInjectionFunc injectFunc, ProfModule module);

    void MsprofEnableMstxFunc(ProfModule module);

private:
    std::string soName_;
    static SHARED_PTR_ALIA<PluginHandle> pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    ProfRegisteMstxFunc profRegisteMstxFunc_ = nullptr;
    EnableMstxFunc enableMstxFunc_ = nullptr;

private:
    void LoadProfCommonSo();

    // get all function addresses at a time
    void GetAllFunction();
};

} // namspace Plugin
} // namspace Dvvp
} // namspace Collector
#endif // PROF_COMMON_PLUGIN_H
