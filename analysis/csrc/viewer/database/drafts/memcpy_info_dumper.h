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

#include "analysis/csrc/viewer/database/drafts/base_dumper.h"
#include "collector/inc/toolchain/prof_common.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using MemcpyInfoData = std::vector<std::tuple<std::string, std::string, uint32_t, uint32_t, uint64_t,
                                       uint64_t, uint16_t, uint64_t, int64_t>>;
class MemcpyInfoDumper : public BaseDumper<MemcpyInfoDumper> {
    using MemcpyInfos = std::vector<std::shared_ptr<MsprofCompactInfo>>;
public:
    explicit MemcpyInfoDumper(const std::string &hostPath);
    MemcpyInfoData GenerateData(const MemcpyInfos &memcpyInfos);
private:
    std::unordered_map<uint64_t, int64_t> GetConnectionId();
};
} // Drafts
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_CSRC_VIEWER_DATABASE_MEMCPY_INFO_DUMPER_H
