#ifndef GE_PLUGIN_H
#define GE_PLUGIN_H

#include "singleton/singleton.h"
#include "ge_profiling.h"
#include "plugin_manager.h"

/* Msprof调用 ge_executor.so 使用的函数原型  
*  ProfSetStepInfo
*/
namespace Analysis {
namespace Dvvp {
namespace Plugin {

class GePlugin : public analysis::dvvp::common::singleton::Singleton<GePlugin> {
public:
    GePlugin()
     :load_(false),
     :soName_("libge_executor.so"),
      pluginManager_(PluginManager(soName_))
    {}
    ~GePlugin() = default;

    // ProfSetStepInfo
    /*ge::Status ProfSetStepInfo(const uint64_t index_id, const uint16_t tag_id, rtStream_t const stream);*/
    using MSPROF_PROFSETSTEPINFO_T = ge::Status (*)(const uint64_t, const uint16_t, rtStream_t);
    ge::Status MsprofProfSetStepInfo(const uint64_t index_id, const uint16_t tag_id, rtStream_t const stream);

private:
    bool load_;
    std::string soName_;
    PluginManager pluginManager_;
};

} // Plugin
} // Dvvp
} // Analysis

#endif