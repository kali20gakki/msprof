#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <string>
#include <functional>
#include <dlfcn.h>

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
    : so_name_(name),
      handle_(nullptr),
      load_(false)
    {}
    ~PluginManager() {}
    const std::string GetSoName() const;
    Status OpenPlugin(const std::string& path);
    Status CloseHandle();
    template <typename R, typename... Types>
    Status GetFunction(const std::string& func_name, std::function<R(Types... args)>& func) const
    {
        func = (R(*)(Types...))dlsym(handle, func_name.c_str());
        if (!func) {
            return PLUGIN_LOAD_FAILED;
        }
        return PLUGIN_LOAD_SUCCESS;
    }
    bool HasLoad();

private:
    std::string so_name_;
    HandleType handle_;
    bool load_;
    std::string RealPath(const std::string &path) const;
};
} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis
#endif