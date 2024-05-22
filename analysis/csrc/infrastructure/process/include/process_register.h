/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_register.h
 * Description        : 流程注册类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_REGISTER_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_REGISTER_H

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
template <typename P>
std::unique_ptr<Infra::Process> TCreator()
{
    std::unique_ptr<Infra::Process> ptr{new P};
    return ptr;
}
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

template<uint32_t ...Ids>
struct ChipIdCountChecker {
    constexpr static size_t count_ = sizeof...(Ids);
};

template <typename... T>
struct ClassToTypeIndexHelper {
    ClassToTypeIndexHelper() : typeIndex{typeid(T) ...} {}
    std::vector<std::type_index> typeIndex;
};
}
}

/**
 * @brief 注册处理流程的自身属性和前身依赖次序
 *
 * @param ProcessClassType: 本处理流程的类名（继承自Infra::Process）
 * @param processCreator：创建接口
 * @param mandatory: 本流程失败后，是否停止后续流程直接失败
 * @param ...: 前向依赖的处理类（可变），如果没有依赖的流程，则不填本参数
 */
#define REGISTER_PROCESS_SEQUENCE(ProcessClassType, mandatory, ...) \
static Infra::ClassToTypeIndexHelper<__VA_ARGS__> proHelper;        \
static Infra::ProcessRegister regSeqInstance(typeid(ProcessClassType), TCreator<ProcessClassType>, \
                                             mandatory, #ProcessClassType, std::move(proHelper.typeIndex))

/**
 * @brief 注册处理流程的依赖的数据，如果没有前身依赖，则不需要使用本宏
 *
 * @param ProcessClassType: 本处理流程的类名（继承自Infra::Process）
 * @param ...: 前向依赖的数据类（可变）
 */
#define REGISTER_PROCESS_DEPENDENT_DATA(ProcessClassType, ...)         \
static Infra::ClassToTypeIndexHelper<__VA_ARGS__> dataTypeHelper;      \
static Infra::ProcessRegister regDepDataInstance(typeid(ProcessClassType), std::move(dataTypeHelper.typeIndex))

/**
 * @brief 注册处理流程的支持的芯片类型
 *
 * @param ProcessClassType: 本处理流程的类名（继承自Infra::Process）
 * @param ...: 依赖的芯片类型（可变）
 */
#define REGISTER_PROCESS_SUPPORT_CHIP(ProcessClassType, ...)                                      \
static_assert(Infra::ChipIdCountChecker<__VA_ARGS__>::count_ != 0, "Do Not Forget the CHIP ID!"); \
static Infra::ProcessRegister regChipIdsInstance(typeid(ProcessClassType), {__VA_ARGS__})

#endif