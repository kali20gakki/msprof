/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memcpy_info_db_dumper.h
 * Description        : modelname落盘
 * Author             : msprof team
 * Creation Date      : 2024/12/9
 * *****************************************************************************
 */
#ifndef ANALYSIS_CSRC_VIEWER_DATABASE_MEMCPY_INFO_DUMPER_H
#define ANALYSIS_CSRC_VIEWER_DATABASE_MEMCPY_INFO_DUMPER_H

#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "collector/inc/toolchain/prof_common.h"

namespace Analysis {
namespace Domain {
using MemcpyInfoData = std::vector<std::tuple<uint32_t, uint16_t, uint16_t, uint32_t, uint16_t, int64_t, uint16_t>>;
using MemcpyInfos = std::vector<std::shared_ptr<MsprofCompactInfo>>;
struct MemcpyAsyncTask {
    uint32_t threadId;
    uint64_t timestamp;
    uint32_t streamId;
    uint16_t taskId;
    uint16_t batchId;
    uint16_t deviceId;
    MemcpyAsyncTask() : threadId(UINT32_MAX), timestamp(0), streamId(UINT32_MAX), taskId(UINT16_MAX),
        batchId(UINT16_MAX), deviceId(UINT16_MAX)
    {}
};

class MemcpyInfoDumper : public BaseDumper<MemcpyInfoDumper> {
public:
    explicit MemcpyInfoDumper(const std::string &hostPath);
    MemcpyInfoData GenerateData(const MemcpyInfos &memcpyInfos);
private:
    std::vector<MemcpyAsyncTask> GetMemcpyAsyncTasks();
};
} // Domain
} // Analysis

#endif // ANALYSIS_CSRC_VIEWER_DATABASE_MEMCPY_INFO_DUMPER_H
