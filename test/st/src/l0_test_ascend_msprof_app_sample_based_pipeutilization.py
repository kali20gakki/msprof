# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
import glob
import sys
import logging

from utils.file_check import FileChecker
from src.base_ascend_msprof_checker import BaseAscendMsprofChecker
from utils.mark_utils import arg_mark

logging.basicConfig(level=logging.INFO,
                    format='\n%(asctime)s %(filename)s [line:%(lineno)d] [%(levelname)s] %(message)s')


class TestAscendMsprofAppSampleBasedPipeUtilization(BaseAscendMsprofChecker):
    def check_ai_vector_core_utilization_data(self):
        ai_vector_core_utilization_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                                    f"ai_vector_core_utilization*.csv")
        if not ai_vector_core_utilization_path:
            raise FileNotFoundError(f"No ai_vector_core_utilization.csv found in {self.prof_path}")
        FileChecker.check_csv_data_non_negative(ai_vector_core_utilization_path[0],
                                                comparison_func=self._is_non_negative,
                                                columns=["vec_ratio", "mac_ratio",
                                                         "scalar_ratio", "mte1_ratio", "mte2_ratio",
                                                         "mte3_ratio", "icache_miss_rate",
                                                         "memory_bound"])

    def check_time_line_data(self):
        super().check_time_line_data()

    def check_op_summary_data(self):
        super().check_op_summary_data()

    def check_op_statistic_data(self):
        super().check_op_statistic_data()

    def check_ai_core_utilization_data(self):
        ai_core_utilization_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/ai_core_utilization"
                                             f"*.csv")
        if not ai_core_utilization_path:
            raise FileNotFoundError(f"No ai_core_utilization.csv found in {self.prof_path}")
        FileChecker.check_csv_data_non_negative(ai_core_utilization_path[0], comparison_func=self._is_non_negative,
                                                columns=["mac_ratio", "scalar_ratio", "mte1_ratio", "mte2_ratio",
                                                         "fixpipe_ratio", "icache_miss_rate"])

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


def test_ascend_msprof_app_sample_based_pipe_utilization(prof_path):
    expect_db_tables = ["ACC_PMU", "AICORE_FREQ", "CANN_API", "COMPUTE_TASK_INFO", "ENUM_API_TYPE",
                        "ENUM_HCCL_DATA_TYPE", "ENUM_HCCL_LINK_TYPE", "ENUM_HCCL_RDMA_TYPE", "ENUM_HCCL_TRANSPORT_TYPE",
                        "ENUM_MEMCPY_OPERATION", "ENUM_MODULE", "ENUM_MSTX_EVENT_TYPE", "HBM", "HOST_INFO", "LLC",
                        "MEMCPY_INFO", "META_DATA", "NPU_INFO", "NPU_MEM", "NPU_MODULE_MEM", "QOS", "RANK_DEVICE_MAP",
                        "SAMPLE_PMU_SUMMARY", "SAMPLE_PMU_TIMELINE", "SESSION_TIME_INFO", "SOC_BANDWIDTH_LEVEL",
                        "STRING_IDS", "TASK"]
    test_ascend_msprof = TestAscendMsprofAppSampleBasedPipeUtilization(prof_path, expect_db_tables)
    test_ascend_msprof.check_msprof_file()


if __name__ == '__main__':
    path = sys.argv[1]
    test_ascend_msprof_app_sample_based_pipe_utilization(path)
