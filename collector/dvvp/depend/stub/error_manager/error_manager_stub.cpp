/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: stub for error manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "error_manager/error_manager.h"

namespace error_message {
int32_t FormatErrorMessage(char_t *str_dst, size_t dst_max, const char_t *format, ...)
{
    return 0;
}
}

thread_local error_message::Context ErrorManager::error_context_ = {0UL, "", "", ""};

ErrorManager &ErrorManager::GetInstance() {
    static ErrorManager instance;
    return instance;
}

error_message::Context &ErrorManager::GetErrorManagerContext() {
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