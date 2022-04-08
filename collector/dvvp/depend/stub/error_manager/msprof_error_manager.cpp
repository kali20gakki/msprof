#include "msprof_error_manager.h"

namespace Analysis {
namespace Dvvp {
namespace MsprofErrMgr {
error_message::Context MsprofErrorManager::errorContext_ = {0UL, "", "", ""};

error_message::Context &MsprofErrorManager::GetErrorManagerContext()
{
    errorContext_ = ErrorManager::GetInstance().GetErrorManagerContext();
    return errorContext_;
}

void MsprofErrorManager::SetErrorContext(const error_message::Context errorContext) const
{
    ErrorManager::GetInstance().SetErrorContext(errorContext);
}
}  // ErrorManager
}  // Dvvp
}  // namespace Analysis