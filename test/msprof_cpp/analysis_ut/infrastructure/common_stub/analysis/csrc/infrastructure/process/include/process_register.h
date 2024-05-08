/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_register.h
 * Description        : fake register head file
 * Author             : msprof team
 * Creation Date      : 2024/4/29
 * *****************************************************************************
 */
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