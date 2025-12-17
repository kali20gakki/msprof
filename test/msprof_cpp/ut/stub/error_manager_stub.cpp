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
#include "common/util/error_manager/error_manager.h"

ErrorManager &ErrorManager::GetInstance()
{
  static ErrorManager instance;
  return instance;
}

error_message::Context &ErrorManager::GetErrorManagerContext()
{
    error_message::Context error_context = {0UL, "", "", ""};
    return error_context;
}
    
void ErrorManager::SetErrorContext(const error_message::Context error_context)
{
    (void)(error_context);
}
