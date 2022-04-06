#include "msprof_error_manager.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
error_message::Context ErrMgrPlugin::errorContext_ = {0UL, "", "", ""};

error_message::Context &ErrMgrPlugin::GetErrorManagerContext() const
{
    return errorContext_;
}

void ErrMgrPlugin::SetErrorContext(const error_message::Context errorContext) const
{
    (void)(errorContext);
}
}  // Plugin
}  // Dvvp
}  // namespace Analysis