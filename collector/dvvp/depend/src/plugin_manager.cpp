#include "plugin_manager.h"
#include "config.h"
#include <limit.h>
#include <cstdlib>

namespace Analysis {
namespace Dvvp {
namespace Plugin {

using namespace analysis::dvvp::common::config;

const string PluginManager::GetSoName() const
{
    return so_name_;
}

std::string RealPath(const std::string &path) const
{
    char resoved_path[MAX_PATH_LENGTH] = {0x00};
    std::string res = "";
    if (realpath(path.c_str(), resoved_path)) {
        res = resoved_path;
    } else {
        MSPROF_LOGE("[RealPath]Get realpath failed.");
    }
    return res;
}

Status PluginManager::OpenPlugin(const std::string& path)
{
    if (path.empty() || path.size() >= MAX_PATH_LENGTH) {
        MSPROF_LOGE("[OpenPlugin]so path wrong");
        return PLUGIN_LOAD_FAILED;
    }
    std::string absoluteDir = RealPath(path);
    if (absoluteDir.empty()) {
        MSPROF_LOGE("[OpenPlugin]Get realpath failed.");
        return PLUGIN_LOAD_FAILED;
    }

    handle_ = dlopen(absoluteDir.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        MSPROF_LOGE("[OpenPlugin]dlopen failed.");
        return PLUGIN_LOAD_FAILED;
    }
    load_ = true;
    return PLUGIN_LOAD_SUCCESS;
}

void PluginManager::CloseHandle()
{
    if (!handle_ || !load_) { // nullptr
        MSPROF_LOGI("[CloseHandle]Do not need to close so.");
        return;
    }
    if (!dlclose(handle)) {
        const char_t *error = dlerror();
        if (!error) {
            MSPROF_LOGE("[CloseHandle]failed to get error msg.");
            return;
        }
        MSPROF_LOGE("[CloseHandle]failed to close [%s]so.msg:[]", so_name_.c_str(), error);
        return;
    }
}

bool PluginManager::HasLoad()
{
    return load_;
}


} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis
