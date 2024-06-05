/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step_trace_persistence.h
 * Description        : step_trace数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/5/23
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_STEP_TRACE_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_STEP_TRACE_PERSISTENCE_H

#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
class StepTracePersistence : public Process {
private:
    uint32_t ProcessEntry(DataInventory &dataInventory, const Context &context) override;
};

}
}
#endif // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_STEP_TRACE_PERSISTENCE_H
