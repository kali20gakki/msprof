/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : context.h
 * Description        : 流程处理上下文基类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_INFRASTRUCTURE_CONTEXT_INCLUDE_CONTEXT_H
#define ANALYSIS_INFRASTRUCTURE_CONTEXT_INCLUDE_CONTEXT_H

#include <string>
#include <cstdint>

namespace Analysis {

namespace Infra {

class Context {
public:
    // Getters for DeviceInfo elements
    virtual const std::string& GetDfxStopAtName() const
    {
        static std::string ins;
        return ins;
    }
    virtual uint32_t GetChipID() const {return {};}

    Context() = default;
    virtual ~Context() = default;
};

}

}

#endif