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

namespace Analysis {
namespace Domain {
namespace {
std::vector<std::string> PipeUtExHeaderString{
    "mac_time", "mac_ratio_extra", "scalar_time", "scalar_ratio", "mte1_time", "mte1_ratio_extra", "mte2_time",
    "mte2_ratio", "fixpipe_time", "fixpipe_ratio", "icache_miss_rate"
};

std::vector<std::string> ArithMetricHeaderString{
    "mac_fp16_ratio", "mac_int8_ratio", "vec_fp32_ratio", "vec_fp16_ratio", "vec_int32_ratio", "vec_misc_ratio",
    "cube_fops", "vector_fops"
};

std::vector<std::string> PipeUtHeaderString{
    "vec_time", "vec_ratio", "mac_time", "mac_ratio", "scalar_time", "scalar_ratio", "mte1_time", "mte1_ratio",
    "mte2_time", "mte2_ratio", "mte3_time", "mte3_ratio", "icache_miss_rate"
};

std::vector<std::string> MemoryHeaderString{
    "ub_read_bw", "ub_write_bw", "l1_read_bw", "l1_write_bw", "main_mem_read_bw", "main_mem_write_bw", "l2_read_bw",
    "l2_write_bw"
};

std::vector<std::string> MemoryL0HeaderString{
    "l0a_read_bw", "l0a_write_bw", "l0b_read_bw", "l0b_write_bw",
    "l0c_read_bw", "l0c_write_bw", "l0c_read_bw_cube", "l0c_write_bw_cube"
};

std::vector<std::string> ResourceHeaderString{
    "vec_bankgroup_cflt_ratio", "vec_bank_cflt_ratio", "vec_resc_cflt_ratio"
};

std::vector<std::string> MemoryUBHeaderString{
    "ub_read_bw_vector", "ub_write_bw_vector", "ub_read_bw_scalar", "ub_write_bw_scalar"
};

std::vector<std::string> L2CacheHeaderString{
    "write_cache_hit", "write_cache_miss_allocate", "r0_read_cache_hit", "r0_read_cache_miss_allocate",
    "r1_read_cache_hit", "r1_read_cache_miss_allocate"
};

std::vector<std::string> MemoryAccessString{
    "read_main_memory_datas", "write_main_memory_datas", "GM_to_L1_datas", "L0C_to_L1_datas", "L0C_to_GM_datas",
    "GM_to_UB_datas", "UB_to_GM_datas"
};
}
std::unordered_map<std::type_index, std::vector<std::string>> Metric::metricMappingStringTable{
    {typeid(PipeUtilizationExctIndex), PipeUtExHeaderString},
    {typeid(ArithMetricIndex), ArithMetricHeaderString},
    {typeid(PipeLineUtIndex), PipeUtHeaderString},
    {typeid(MemoryIndex), MemoryHeaderString},
    {typeid(MemoryL0Index), MemoryL0HeaderString},
    {typeid(ResourceConflictIndex), ResourceHeaderString},
    {typeid(MemoryUBIndex), MemoryUBHeaderString},
    {typeid(L2CacheIndex), L2CacheHeaderString},
    {typeid(MemoryAccessIndex), MemoryAccessString}
};
}
}