/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_struct.h
 * Description        : 流程注册类需要使用的结构体定义
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_STRUCT_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_STRUCT_H

#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis {

namespace Infra {

using ProcessCreator = std::function<std::unique_ptr<Process>()>;

struct RegProcessInfo {
    ProcessCreator creator;  // 本Process的创建函数
    std::vector<std::type_index> paramTypes;  // 本Process需要的数据类型
    std::vector<std::type_index> processDependence;  // 本Process依赖的前向Process
    std::vector<uint32_t> chipIds;  // 本Process支持的ChipId
    std::string processName;  // 本Process的类名for Dfx
    bool mandatory;  // 是否为关键流程，关键流程失败后，会停止后续流程导致整体失败 true表示关键流程
};

using ProcessCollection = std::unordered_map<std::type_index, RegProcessInfo>;

class ProcessRegister {
public:
    ProcessRegister(std::type_index selfType, ProcessCreator creator, bool mandatory,
                    const char* processName,
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


#endif