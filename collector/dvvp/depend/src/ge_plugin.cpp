#include "ge_plugin.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
ge::Status GePlugin::MsprofProfSetStepInfo(const uint64_t index_id, const uint16_t tag_id, rtStream_t const stream)
{
    MSPROF_PROFSETSTEPINFO_T func;
    Status ret = pluginManager_.GetFunction<const uint64_t, const uint16_t, rtStream_t>("ProfSetStepInfo", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(index_id, tag_id, stream);
}

} // Plugin
} // Dvvp
} // Analysis