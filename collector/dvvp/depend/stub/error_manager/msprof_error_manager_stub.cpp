/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: wrapper for error manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
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