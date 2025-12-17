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
using ProfRegisterMstxFunc = std::function<void(MstxInitInjectionFunc, ProfModule)>;
using EnableMstxFunc = std::function<void(ProfModule)>;

class ProfCommonPlugin : public analysis::dvvp::common::singleton::Singleton<ProfCommonPlugin> {
public:
    ProfCommonPlugin() : soName_("libprof_common.so"), loadFlag_(0) {}

    bool IsFuncExist(const std::string &funcName) const;

    void MsprofProfRegisterMstxFunc(MstxInitInjectionFunc injectFunc, ProfModule module);

    void MsprofEnableMstxFunc(ProfModule module);

private:
    std::string soName_;
    static SHARED_PTR_ALIA<PluginHandle> pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    ProfRegisterMstxFunc profRegisterMstxFunc_ = nullptr;
    EnableMstxFunc enableMstxFunc_ = nullptr;

private:
    void LoadProfCommonSo();

    // get all function addresses at a time
    void GetAllFunction();
};

} // namespace Plugin
} // namespace Dvvp
} // namespace Collector
#endif // PROF_COMMON_PLUGIN_H
