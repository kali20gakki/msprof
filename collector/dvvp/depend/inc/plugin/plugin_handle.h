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
#ifndef PLUGIN_HANDLE_H
#define PLUGIN_HANDLE_H

#include <string>
#include <vector>
#include <functional>
#include <dlfcn.h>
#include "errno/error_code.h"
#include "utils.h"

#define PTHREAD_ONCE_T pthread_once_t
namespace Collector {
namespace Dvvp {
namespace Plugin {
using namespace analysis::dvvp::common::utils;
using HandleType = void*;
using analysis::dvvp::common::error::PROFILING_SUCCESS;
using analysis::dvvp::common::error::PROFILING_FAILED;
inline void PthreadOnce(pthread_once_t *flag, void (*func)(void))
{
    (void)pthread_once(flag, func);
}
class PluginHandle {
public:
    explicit PluginHandle(const std::string &name) : soName_(name), handle_(nullptr), load_(false) {}
    ~PluginHandle();
    const std::string GetSoName() const;
    int32_t OpenPluginFromEnv(const std::string envValue, bool &isSoValid);
    int32_t OpenPluginFromLdcfg(bool &isSoValid);
    void CloseHandle();
    template <typename R, typename... Types>
    void GetFunction(const std::string& funcName, std::function<R(Types... args)>& func) const
    {
        func = (R(*)(Types...))dlsym(handle_, funcName.c_str());
    }
    HandleType GetFunctionForC(const std::string& funcName) const
    {
        return dlsym(handle_, funcName.c_str());
    }
    bool HasLoad();
    bool IsFuncExist(const std::string funcName) const;

protected:
    std::vector<std::string> soPaths_;

private:
    void GetSoPaths(std::vector<std::string> &soPaths);
    std::string GetSoPath(const std::string &envValue) const;
    bool CheckSoValid(const std::string& soPath) const;
    int32_t DlopenSo(const std::string& soPath);
    std::string GetAscendHalPath() const;
    std::string soName_;
    HandleType handle_;
    bool load_;
};
} // Plugin
} // Dvvp
} // Collector
#endif