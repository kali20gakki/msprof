/**
 * @file msprof_error_manager.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
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