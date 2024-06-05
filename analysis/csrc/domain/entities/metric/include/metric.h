/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : metric.h
 * Description        : pmu metric结构
 * Author             : msprof team
 * Creation Date      : 2024/5/9
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_ENTITIES_METRIC_METRIC_H
#define ANALYSIS_DOMAIN_ENTITIES_METRIC_METRIC_H

#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "analysis/csrc/dfx/log.h"

namespace Analysis {
namespace Domain {
const std::string INVALID_HEADER = "INVALID";
// PipeUtilizationExct对应910B芯片的AIV的PipeUtilization,两者属于等价分组
enum class PipeUtilizationExctIndex {
    // 计算该分组的PMU时，以ratio/ratio_extra结尾的metric需要计算时间
    MacTime = 0,
    MacRatioExtra,
    ScalarTime,
    ScalarRatio,
    Mte1Time,
    Mte1RatioExtra,
    Mte2Time,
    Mte2Ratio,
    FixPipeTime,
    FixPipeRatio,
    ICacheMissRate,
};

enum class ArithMetricIndex {
    MacFp16Ratio = 0,
    MacInt8Ratio,
    VecFp32Ratio,
    VecFp16Ratio,
    VecInt32Ratio,
    VecMiscRatio,
    CubeFops,
    VectorFops,
};

enum class PipeLineUtIndex {
    // 计算该分组的PMU时，以ratio/ratio_extra结尾的metric需要计算时间
    VecTime = 0,
    VecRatio,
    MacTime,
    MacRatio,
    ScalarTime,
    ScalarRatio,
    Mte1Time,
    Mte1Ratio,
    Mte2Time,
    Mte2Ratio,
    Mte3Time,
    Mte3Ratio,
    ICacheMissRate,
};

enum class MemoryIndex {
    UBReadBw = 0,
    UBWriteBw,
    L1ReadBw,
    L1WriteBw,
    MainMemReadBw,
    MainMemWriteBw,
};

enum class MemoryL0Index {
    L0aReadBw = 0,
    L0aWriteBw,
    L0bReadBw,
    L0bWriteBw,
    L0cReadBw,
    L0cWriteBw,
    L0cReadBwCube,
    L0cWriteBwCube,
};

enum class ResourceConflictIndex {
    VecBankGroupCfltRatio = 0,
    VecBankCfltRatio,
    VecRescCfltRatio,
};

enum class MemoryUBIndex {
    UbReadBwVector = 0,
    UbWriteBwVector,
    UbReadBwScalar,
    UbWriteBwScalar,
};

enum class L2CacheIndex {
    WriteCacheHit = 0,
    WriteCacheMissAllocate,
    R0ReadCacheHit,
    R0ReadCacheMissAllocate,
    R1ReadCacheHit,
    R1ReadCacheMissAllocate,
};

class Metric {
public:
    template<typename T>
    static const std::string &GetMetricHeaderString(T enumType)
    {
        auto it = metricMappingStringTable.find(typeid(enumType));
        if (it == metricMappingStringTable.end()) {
            ERROR("Invalid enumType");
            return INVALID_HEADER;
        }
        return it->second[static_cast<int>(enumType)];
    }
private:
    static std::unordered_map<std::type_index, std::vector<std::string>> metricMappingStringTable;
};
}
}
#endif // ANALYSIS_DOMAIN_ENTITIES_METRIC_METRIC_H
