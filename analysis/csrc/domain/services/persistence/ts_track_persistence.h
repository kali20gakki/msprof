/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ts_track_persistence.h
 * Description        : ts_track中间状态数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/6/3
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_TS_TRACK_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_TS_TRACK_PERSISTENCE_H

#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
class TsTrackPersistence : public Process {
private:
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_TS_TRACK_PERSISTENCE_H
