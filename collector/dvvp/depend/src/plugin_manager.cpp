#include "plugin_manager.h"
#include "config.h"

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
    }
    return res;
}

Status PluginManager::OpenPlugin(const std::string& path)
{
    if (path.empty() || path.size() >= MAX_PATH_LENGTH) {
        return PLUGIN_LOAD_FAILED;
    }
    std::string absoluteDir = RealPath(path);
    if (absoluteDir.empty()) {
        return PLUGIN_LOAD_FAILED;
    }

    handle_ = dlopen(absoluteDir.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        return PLUGIN_LOAD_FAILED;
    }
    load_ = true;
    return PLUGIN_LOAD_SUCCESS;
}

Status PluginManager::CloseHandle()
{
    if (!handle_) {
        // [To add message]
        return;
    }
    if (!dlclose(handle)) {
        // [To add message]
        return PLUGIN_LOAD_FAILED;
    }
    return PLUGIN_LOAD_SUCCESS;
}

bool PluginManager::HasLoad()
{
    return load_;
}


} // namespace Plugin
} // namespace Dvvp
} // namespace Analysis
