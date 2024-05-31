/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_task_association.h
 * Description        : Ascend_task关联
 * Author             : msprof team
 * Creation Date      : 2024/5/23
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_ASCEND_TASK_ASSOCIATION_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_ASCEND_TASK_ASSOCIATION_H

#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
class AscendTaskAssociation : public Process {
private:
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_ASCEND_TASK_ASSOCIATION_H
