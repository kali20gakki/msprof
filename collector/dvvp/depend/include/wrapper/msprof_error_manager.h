/**
 * @file msprof_error_manager.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef MSPROF_ERROR_MANAGER_H
#define MSPROF_ERROR_MANAGER_H

#include "common/util/error_manager/error_manager.h"
#include "common/singleton/singleton.h"

#define MSPROF_INPUT_ERROR REPORT_INPUT_ERROR
#define MSPROF_ENV_ERROR REPORT_ENV_ERROR
#define MSPROF_INNER_ERROR REPORT_INNER_ERROR
#define MSPROF_CALL_ERROR REPORT_CALL_ERROR

namespace Analysis {
namespace Dvvp {
namespace MsprofErrMgr {

class MsprofErrorManager : public analysis::dvvp::common::singleton::Singleton<MsprofErrorManager> {
public:
    error_message::Context &GetErrorManagerContext();
    void SetErrorContext(const error_message::Context errorContext) const;
    MsprofErrorManager() {}
    ~MsprofErrorManager() override {}
private:
    static error_message::Context errorContext_;
};

}  // ErrorManager
}  // Dvvp
}  // namespace Analysis
#endif