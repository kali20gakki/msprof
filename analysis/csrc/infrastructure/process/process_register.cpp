/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_register.cpp
 * Description        : 流程注册类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */

#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis {

namespace Infra {

ProcessRegister::ProcessRegister(std::type_index selfType, ProcessCreator creator,
                                 bool mandatory, const char* processName,
                                 std::vector<std::type_index>&& preProcessType)
{
    GetContainer()[selfType].creator = creator;
    GetContainer()[selfType].mandatory = mandatory;
    GetContainer()[selfType].processDependence = preProcessType;
    GetContainer()[selfType].processName = processName;
}

ProcessRegister::ProcessRegister(std::type_index selfType, std::vector<std::type_index>&& paramTypes)
{
    GetContainer()[selfType].paramTypes = paramTypes;
}

ProcessRegister::ProcessRegister(std::type_index selfType, std::initializer_list<uint32_t> chipIds)
{
    GetContainer()[selfType].chipIds = chipIds;
}

ProcessCollection& ProcessRegister::GetContainer()
{
    static ProcessCollection container;
    return container;
}

ProcessCollection ProcessRegister::CopyProcessInfo()
{
    // 注册信息数据量小，这里COPY一份
    return GetContainer();
}

}

}