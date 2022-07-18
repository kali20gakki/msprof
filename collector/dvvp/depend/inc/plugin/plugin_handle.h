/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: dlopen interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#ifndef PLUGIN_HANDLE_H
#define PLUGIN_HANDLE_H

#include <string>
#include <vector>
#include <functional>
#include <dlfcn.h>
#include "errno/error_code.h"

#define PTHREAD_ONCE_T pthread_once_t
namespace Collector {
namespace Dvvp {
namespace Plugin {
using HandleType = void*;
using analysis::dvvp::common::error::PROFILING_SUCCESS;
using analysis::dvvp::common::error::PROFILING_FAILED;
inline void PthreadOnce(pthread_once_t *flag, void (*func)(void))
{
    (void)pthread_once(flag, func);
}
class PluginHandle {
public:
    explicit PluginHandle(const std::string &name)
    : soName_(name),
      handle_(nullptr),
      load_(false)
    {}
    ~PluginHandle();
    const std::string GetSoName() const;
    int32_t OpenPlugin(const std::string envValue);
    void CloseHandle();
    template <typename R, typename... Types>
    int32_t GetFunction(const std::string& funcName, std::function<R(Types... args)>& func) const
    {
        func = (R(*)(Types...))dlsym(handle_, funcName.c_str());
        if (!func) {
            return PROFILING_FAILED;
        }
        return PROFILING_SUCCESS;
    }
    bool HasLoad();
    bool IsFuncExist(const std::string funcName) const;

private:
    std::string GetSoPath(const std::string &envValue) const;
    void SplitPath(const std::string &mutilPath, std::vector<std::string> &patVec) const;
    std::string soName_;
    HandleType handle_;
    bool load_;
};
} // Plugin
} // Dvvp
} // Collector
#endif