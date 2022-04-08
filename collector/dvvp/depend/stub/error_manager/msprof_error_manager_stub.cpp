#include "msprof_error_manager.h"

namespace Analysis {
namespace Dvvp {
namespace MsprofErrMgr {
error_message::Context MsprofErrorManager::errorContext_ = {0UL, "", "", ""};

error_message::Context &MsprofErrorManager::GetErrorManagerContext() const
{
    return errorContext_;
}

void MsprofErrorManager::SetErrorContext(const error_message::Context errorContext) const
{
    (void)(errorContext);
}
}  // ErrorManager
}  // Dvvp
}  // namespace Analysis