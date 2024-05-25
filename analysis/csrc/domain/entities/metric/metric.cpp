/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : metric.cpp
 * Description        : pmu metric结构
 * Author             : msprof team
 * Creation Date      : 2024/5/9
 * *****************************************************************************
 */

#include "analysis/csrc/domain/entities/metric/include/metric.h"
#include <unordered_map>
#include <vector>
#include "analysis/csrc/dfx/log.h"

namespace Analysis {
namespace Domain {
namespace {
const std::string INVALID_HEADER = "INVALID";
std::vector<std::string> PipeUtExHeaderString{
    "mac_ratio_extra", "scalar_ratio", "mte1_ratio_extra", "mte2_ratio", "fixpipe_ratio", "icache_miss_rate",
    "mac_time", "scalar_time", "mte1_time", "mte2_time", "fixpipe_time"
};

std::vector<std::string> ArithMetricHeaderString{
    "mac_fp16_ratio", "mac_int8_ratio", "vec_fp32_ratio", "vec_fp16_ratio", "vec_int32_ratio", "vec_misc_ratio",
    "cube_fops", "vector_fops"
};

std::vector<std::string> PipeUtHeaderString{
    "vec_ratio", "mac_ratio", "scalar_ratio", "mte1_ratio", "mte2_ratio", "mte3_ratio", "icache_miss_rate",
    "vec_time", "mac_time", "scalar_time", "mte1_time", "mte2_time", "mte3_time"
};

std::vector<std::string> MemoryHeaderString{
    "ub_read_bw(GB/s)", "ub_write_bw(GB/s)", "l1_read_bw(GB/s)", "l1_write_bw(GB/s)",
    "main_mem_read_bw(GB/s)", "main_mem_write_bw(GB/s)"
};

std::vector<std::string> MemoryL0HeaderString{
    "l0a_read_bw(GB/s)", "l0a_write_bw(GB/s)", "l0b_read_bw(GB/s)", "l0b_write_bw(GB/s)",
    "l0c_read_bw(GB/s)", "l0c_write_bw(GB/s)", "l0c_read_bw_cube(GB/s)", "l0c_write_bw_cube(GB/s)"
};

std::vector<std::string> ResourceHeaderString{
    "vec_bankgroup_cflt_ratio", "vec_bank_cflt_ratio", "vec_resc_cflt_ratio"
};

std::vector<std::string> MemoryUBHeaderString{
    "ub_read_bw_vector(GB/s)", "ub_write_bw_vector(GB/s)", "ub_read_bw_scalar(GB/s)", "ub_write_bw_scalar(GB/s)"
};

std::vector<std::string> L2CacheHeaderString{
    "write_cache_hit", "write_cache_miss_allocate", "r0_read_cache_hit", "r0_read_cache_miss_allocate",
    "r1_read_cache_hit", "r1_read_cache_miss_allocate"
};

std::unordered_map<std::type_index, std::vector<std::string>> metricMappingStringTable{
    {typeid(PipeUtilizationExctIndex), PipeUtExHeaderString},
    {typeid(ArithMetricIndex), ArithMetricHeaderString},
    {typeid(PipeLineUtIndex), PipeUtHeaderString},
    {typeid(MemoryIndex), MemoryHeaderString},
    {typeid(MemoryL0Index), MemoryL0HeaderString},
    {typeid(ResourceConflictIndex), ResourceHeaderString},
    {typeid(MemoryUBIndex), MemoryUBHeaderString},
    {typeid(L2CacheIndex), L2CacheHeaderString},
};
}

template<typename T>
const std::string& GetMetricHeaderString(T enumType)
{
    auto it = metricMappingStringTable.find(typeid(enumType));
    if (it == metricMappingStringTable.end()) {
        ERROR("Invalid enumType");
        return INVALID_HEADER;
    }
    return it->second[static_cast<int>(enumType)];
}

}
}