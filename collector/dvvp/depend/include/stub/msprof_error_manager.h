/**
 * @file msprof_error_manager.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef MSPROF_ERROR_MANAGER_STUB_H
#define MSPROF_ERROR_MANAGER_STUB_H

#include "common/util/error_manager/error_manager.h"
#include "common/singleton/singleton.h"

#define MSPROF_INPUT_ERROR(error_code, key, value)
#define MSPROF_ENV_ERROR(error_code, key, value)
#define MSPROF_INNER_ERROR(error_code, fmt, ...)
#define MSPROF_CALL_ERROR MSPROF_INNER_ERROR

namespace Analysis {
namespace Dvvp {
namespace MsprofErrMgr {

class MsprofErrorManager : public analysis::dvvp::common::singleton::Singleton<MsprofErrorManager> {
public:
    void SetErrorContext(const error_message::Context errorContext) const;
    error_message::Context &GetErrorManagerContext() const;
    MsprofErrorManager() {}
    ~MsprofErrorManager() override {}
private:
    static error_message::Context errorContext_;
};

}  // ErrorManager
}  // Dvvp
}  // namespace Analysis
#endif