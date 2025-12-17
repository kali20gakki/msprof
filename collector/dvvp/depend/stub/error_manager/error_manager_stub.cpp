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
#include "error_manager/error_manager.h"

namespace error_message {
int32_t FormatErrorMessage(char_t *str_dst, size_t dst_max, const char_t *format, ...)
{
    return 0;
}
}

thread_local error_message::Context ErrorManager::error_context_ = {0UL, "", "", ""};

ErrorManager &ErrorManager::GetInstance()
{
    static ErrorManager instance;
    return instance;
}

error_message::Context &ErrorManager::GetErrorManagerContext()
{
    return error_context_;
}

void ErrorManager::ATCReportErrMessage(const std::string error_code, const std::vector<std::string> &key,
                                       const std::vector<std::string> &value)
{}

int32_t ErrorManager::ReportInterErrMessage(const std::string error_code, const std::string &error_msg)
{
    return 0;
}

void ErrorManager::SetErrorContext(error_message::Context error_context)
{}