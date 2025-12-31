# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import sys
import logging

from src.base_ascend_msprof_checker import BaseAscendMsprofChecker

logging.basicConfig(level=logging.INFO,
                    format='\n%(asctime)s %(filename)s [line:%(lineno)d] [%(levelname)s] %(message)s')


class TestAscendMsprofAppAicModeSampleBasedReadMemory(BaseAscendMsprofChecker):
    def check_ai_vector_core_utilization_data(self):
        super().check_ai_vector_core_utilization_data()

    def check_time_line_data(self):
        super().check_time_line_data()

    def check_op_summary_data(self):
        super().check_op_summary_data()

    def check_op_statistic_data(self):
        super().check_op_statistic_data()

    def check_ai_core_utilization_data(self):
        super().check_ai_core_utilization_data()

    def check_npu_mem_data(self):
        super().check_npu_mem_data()

    def check_npu_module_mem_data(self):
        super().check_npu_module_mem_data()

    def check_soc_pmu_data(self):
        super().check_soc_pmu_data()

    def check_task_time_data(self):
        super().check_task_time_data()

    def check_api_statistic_data(self):
        super().check_api_statistic_data()

    def check_hbm_data(self):
        super().check_hbm_data()

    def check_l2_cache_data(self):
        super().check_l2_cache_data()

    def check_llc_read_write_data(self):
        super().check_llc_read_write_data()

    def check_other_data(self):
        super().check_other_data()


def test_ascend_msprof_aic_mode_sample_based_read_memory(prof_path):
    expect_db_tables = ["AICORE_FREQ", "CANN_API", "COMPUTE_TASK_INFO", "ENUM_API_TYPE", "ENUM_HCCL_DATA_TYPE",
                        "ENUM_HCCL_LINK_TYPE", "ENUM_HCCL_RDMA_TYPE", "ENUM_HCCL_TRANSPORT_TYPE",
                        "ENUM_MEMCPY_OPERATION", "ENUM_MODULE", "ENUM_MSTX_EVENT_TYPE", "HOST_INFO", "META_DATA",
                        "NPU_INFO", "RANK_DEVICE_MAP", "SESSION_TIME_INFO", "STRING_IDS", "TASK", "TASK_PMU_INFO"]
    test_ascend_msprof = TestAscendMsprofAppAicModeSampleBasedReadMemory(prof_path, expect_db_tables)
    test_ascend_msprof.check_msprof_file()


if __name__ == '__main__':
    path = sys.argv[1]
    test_ascend_msprof_aic_mode_sample_based_read_memory(path)
