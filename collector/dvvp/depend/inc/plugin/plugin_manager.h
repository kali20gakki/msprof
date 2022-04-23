#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <dlfcn.h>
#include "msprof_dlog.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {

using Status = uint32_t;
using HandleType = void*;
const Status PLUGIN_LOAD_SUCCESS = 0x0;
const Status PLUGIN_LOAD_FAILED = 0xFFFFFFFF;

class PluginManager {
public:
    explicit PluginManager(const std::string &name)
    : soName_(name),
      handle_(nullptr),
      load_(false)
    {}
    ~PluginManager() {}
    const std::string GetSoName() const;
    Status OpenPlugin(const std::string envValue);
    void CloseHandle();
    template <typename R, typename... Types>
    Status GetFunction(const std::string& funcName, std::function<R(Types... args)>& func) const
    {
        func = (R(*)(Types...))dlsym(handle_, funcName.c_str());
        if (!func) {
            MSPROF_LOGE("[GetFunction]Get function from so failed.");
            return PLUGIN_LOAD_FAILED;
        }
        return PLUGIN_LOAD_SUCCESS;
    }
    bool HasLoad();

private:
    std::string RealPath(const std::string &path) const;
    bool IsFuncExist(const std::string funcName) const;
    std::string GetSoPath(const std::string &envValue) const;
    void SplitPath(const std::string &mutilPath, std::vector<std::string> &patVec) const;
    void ReadDriverConf(std::vector<std::string> &pathVec) const; // "/etc/ld.so.conf.d/ascend_driver_so.conf"
    std::string soName_;
    HandleType handle_;
    bool load_;
};
} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis
#endif