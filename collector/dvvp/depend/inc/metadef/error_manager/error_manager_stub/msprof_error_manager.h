/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef MSPROF_ERROR_MANAGER_STUB_H
#define MSPROF_ERROR_MANAGER_STUB_H

#include "metadef/error_manager/error_manager.h"
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