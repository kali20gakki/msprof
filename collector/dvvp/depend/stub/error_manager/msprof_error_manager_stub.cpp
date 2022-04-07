#include "msprof_error_manager.h"

namespace Analysis {
namespace Dvvp {
namespace MsprofErrMgr {
error_message::Context MsprofErrorManager::errorContext_ = {0UL, "", "", ""};

MsprofErrorManager &MsprofErrorManager::GetInstance()
{
    static MsprofErrorManager instance;
    return instance;
}
error_message::Context &MsprofErrorManager::GetErrorManagerContext()
{
    return errorContext_;
}

void MsprofErrorManager::SetErrorContext(const error_message::Context errorContext)
{
    (void)(errorContext);
}
}  // ErrorManager
}  // Dvvp
}  // namespace Analysis