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
#ifndef UT_TEST_ANALYSIS_INFRASTRUCTURE_PROCESS_REGISTER_H
#define UT_TEST_ANALYSIS_INFRASTRUCTURE_PROCESS_REGISTER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <initializer_list>
#include <vector>
#include <map>
#include <unordered_map>
#include <typeindex>
#include <string>
#include "analysis/csrc/infrastructure/process/include/process_struct.h"

namespace Analysis {
namespace Infra {
class ProcessRegister {
public:
    ProcessRegister(std::type_index selfType, ProcessCreator creator, bool mandatory, const char* processName,
                    std::vector<std::type_index>&& preProcessType);
    ProcessRegister(std::type_index selfType, std::vector<std::type_index>&& paramTypes);
    ProcessRegister(std::type_index selfType, std::initializer_list<uint32_t> chipIds);

    static ProcessCollection CopyProcessInfo();

private:
    static ProcessCollection& GetContainer();
};
}
}

/**
 * @brief 注册处理流程的桩
 */
#define REGISTER_PROCESS_SEQUENCE(ProcessClassType, mandatory, ...)

/**
 * @brief 注册处理流程的桩
 */
#define REGISTER_PROCESS_DEPENDENT_DATA(ProcessClassType, ...)

/**
 * @brief 注册处理流程的桩
 */
#define REGISTER_PROCESS_SUPPORT_CHIP(ProcessClassType, ...)

#endif