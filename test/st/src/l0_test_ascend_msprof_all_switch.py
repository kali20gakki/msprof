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


class TestAscendMsprofAllSwitch(BaseAscendMsprofChecker):
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
        self.check_dvpp_data()
        self.check_hccs_data()
        self.check_nic_data()
        self.check_pcie_data()
        self.check_roce_data()
        self.check_ts_cpu_pmu_events_data()
        self.check_ts_cpu_top_function_data()

    def check_dvpp_data(self):
        dvpp_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                              f"dvpp_*.csv")
        if not dvpp_path:
            raise FileNotFoundError(f"No dvpp.csv found in {self.prof_path}")
        FileChecker.check_csv_data_non_negative(dvpp_path[0], comparison_func=self._is_non_negative,
                                                columns=["Dvpp Id", "Engine ID", "All Time(us)", "All Frame",
                                                         "All Utilization(%)"])

    def check_hccs_data(self):
        hccs_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                              f"hccs_*.csv")
        if not hccs_path:
            raise FileNotFoundError(f"No hccs.csv found in {self.prof_path}")
        FileChecker.check_csv_data_non_negative(hccs_path[0], comparison_func=self._is_non_negative,
                                                columns=["Max", "Min", "Average"])

    def check_nic_data(self):
        nic_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                             f"nic_*.csv")
        if not nic_path:
            raise FileNotFoundError(f"No nic.csv found in {self.prof_path}")
        FileChecker.check_csv_data_non_negative(nic_path[0], comparison_func=self._is_non_negative,
                                                columns=["Timestamp(us)", "Bandwidth(MB/s)",
                                                         "Rx Bandwidth efficiency(%)", "rxPacket/s", "rxError rate(%)",
                                                         "rxDropped rate(%)", "Tx Bandwidth efficiency(%)",
                                                         "txPacket/s", "txError rate(%)", "txDropped rate(%)",
                                                         "funcId"])

    def check_pcie_data(self):
        pcie_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                              f"pcie_*.csv")
        if not pcie_path:
            raise FileNotFoundError(f"No pcie.csv found in {self.prof_path}")
        FileChecker.check_csv_data_non_negative(pcie_path[0], comparison_func=self._is_non_negative,
                                                columns=["Max", "Min", "Average"])

    def check_roce_data(self):
        roce_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                              f"roce_*.csv")
        if not roce_path:
            raise FileNotFoundError(f"No roce.csv found in {self.prof_path}")
        FileChecker.check_csv_data_non_negative(roce_path[0], comparison_func=self._is_non_negative,
                                                columns=["Timestamp(us)", "Bandwidth(MB/s)",
                                                         "Rx Bandwidth efficiency(%)", "rxPacket/s", "rxError rate(%)",
                                                         "rxDropped rate(%)", "Tx Bandwidth efficiency(%)",
                                                         "txPacket/s", "txError rate(%)", "txDropped rate(%)",
                                                         "funcId"])

    def check_ts_cpu_pmu_events_data(self):
        ts_cpu_pmu_events_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                           f"ts_cpu_pmu_events_*.csv")
        if not ts_cpu_pmu_events_path:
            raise FileNotFoundError(f"No ts_cpu_pmu_events.csv found in {self.prof_path}")
        FileChecker.check_csv_headers(ts_cpu_pmu_events_path[0], ["Device_id", "Event", "Name", "Count"])
        FileChecker.check_csv_data_non_negative(ts_cpu_pmu_events_path[0], comparison_func=self._is_non_negative,
                                                columns=["Count"])

    def check_ts_cpu_top_function_data(self):
        ts_cpu_top_function_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                             f"ts_cpu_top_function_*.csv")
        if not ts_cpu_top_function_path:
            raise FileNotFoundError(f"No ts_cpu_pmu_events.csv found in {self.prof_path}")
        FileChecker.check_csv_headers(ts_cpu_top_function_path[0], ["Device_id", "Function", "Cycles", "Cycles(%)"])
        FileChecker.check_csv_data_non_negative(ts_cpu_top_function_path[0], comparison_func=self._is_non_negative,
                                                columns=["Cycles", "Cycles(%)"])


@arg_mark(level_mark='level1')
def test_ascend_msprof_all_switch(prof_path):
    expect_db_tables = ["ACC_PMU", "AICORE_FREQ", "CANN_API", "COMPUTE_TASK_INFO", "ENUM_API_TYPE",
                        "ENUM_HCCL_DATA_TYPE", "ENUM_HCCL_LINK_TYPE", "ENUM_HCCL_RDMA_TYPE", "ENUM_HCCL_TRANSPORT_TYPE",
                        "ENUM_MEMCPY_OPERATION", "ENUM_MODULE", "ENUM_MSTX_EVENT_TYPE", "HBM", "HCCS", "HOST_INFO",
                        "LLC", "MEMCPY_INFO", "META_DATA", "NETDEV_STATS", "NIC", "NPU_INFO", "NPU_MEM",
                        "NPU_MODULE_MEM", "PCIE", "QOS", "RANK_DEVICE_MAP", "ROCE", "SESSION_TIME_INFO",
                        "SOC_BANDWIDTH_LEVEL", "STRING_IDS", "TASK", "TASK_PMU_INFO"]
    test_ascend_msprof = TestAscendMsprofAllSwitch(prof_path, expect_db_tables)
    test_ascend_msprof.check_msprof_file()


if __name__ == '__main__':
    path = sys.argv[1]
    test_ascend_msprof_all_switch(path)
