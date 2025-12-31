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
from abc import ABC, abstractmethod
import glob
import logging

from utils.db_check import DBManager
from utils.file_check import FileChecker
from utils.table_fields import TableFields

logging.basicConfig(level=logging.INFO,
                    format='\n%(asctime)s %(filename)s [line:%(lineno)d] [%(levelname)s] %(message)s')


def _is_non_negative(value):
    """ Check if a given value is non-negative (i.e., greater than or equal to zero)."""
    return float(value) >= 0.0


class BaseAscendMsprofChecker(ABC):

    def __init__(self, prof_path, expect_db_tables):
        self.prof_path = prof_path
        self.expect_db_tables = expect_db_tables

    def check_msprof_file(self):
        self.check_msprof_text_file()
        self.check_msprof_db_file()

    def check_msprof_text_file(self):
        # 1. msprof.json
        self.check_time_line_data()
        # 2. op_summary.csv
        self.check_op_summary_data()
        # 3. op_statistic.csv
        self.check_op_statistic_data()
        # 4. ai_core_utilization.csv
        self.check_ai_core_utilization_data()
        # 5. ai_vector_core_utilization.csv
        self.check_ai_vector_core_utilization_data()
        # 6. api_statistic.csv
        self.check_api_statistic_data()
        # 7. hbm.csv
        self.check_hbm_data()
        # 8. l2_cache.csv
        self.check_l2_cache_data()
        # 9. llc_read_write.csv
        self.check_llc_read_write_data()
        # 10.npu_mem.csv
        self.check_npu_mem_data()
        # 10.npu_module_mem.csv
        self.check_npu_module_mem_data()
        # 11.soc_pmu.csv
        self.check_soc_pmu_data()
        # 12.task_time.csv
        self.check_task_time_data()
        # 13. other data
        self.check_other_data()
        # 14. profiler.log
        self.check_msprof_log()

    @abstractmethod
    def check_time_line_data(self):
        msprof_json_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/msprof_*.json")
        assert msprof_json_path, f"No msprof.json found in {self.prof_path}"
        FileChecker.check_timeline_values(msprof_json_path[0], "cat", ["HostToDevice", ], fuzzy_match=True)
        FileChecker.check_timeline_values(msprof_json_path[0], "name", ["aclnnAdd_*", "Conv2D*", "*Relu*"])

    @abstractmethod
    def check_op_summary_data(self):
        op_summary_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/op_summary_*.csv")
        assert op_summary_path, f"No op_summary.csv found in {self.prof_path}"
        FileChecker.check_csv_items(op_summary_path[0], {"Op Name": ["aclnnAdd_*", "Conv2D*", "*Relu*"]})
        FileChecker.check_csv_items(op_summary_path[0], {"OP Type": ["Add", "Conv2D", "Relu"],
                                                         "Task Type": ["AI_VECTOR_CORE", "AI_CORE", "MIX_AIV"]},
                                    fuzzy_match=False)
        FileChecker.check_csv_data_non_negative(op_summary_path[0], comparison_func=_is_non_negative,
                                                columns=["Task Duration(us)", "Task Wait Time(us)",
                                                         "Task Start Time(us)", "Block Dim", "Mix Block Dim",
                                                         "Task ID", "Stream ID", "Device_id", "Model ID"])

    @abstractmethod
    def check_op_statistic_data(self):
        op_statistic_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/op_statistic_*.csv")
        assert op_statistic_path, f"No op_statistic.csv found in {self.prof_path}"
        FileChecker.check_csv_items(op_statistic_path[0], {"OP Type": ["Add", "Conv2D", "Relu"],
                                                           "Core Type": ["AI_VECTOR_CORE", "AI_CORE", "MIX_AIV"]},
                                    fuzzy_match=False)
        FileChecker.check_csv_data_non_negative(op_statistic_path[0], comparison_func=_is_non_negative,
                                                columns=["Count", "Total Time(us)",
                                                         "Min Time(us)", "Avg Time(us)", "Max Time(us)",
                                                         "Ratio(%)"])

    @abstractmethod
    def check_ai_core_utilization_data(self):
        ai_core_utilization_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/ai_core_utilization"
                                             f"*.csv")
        assert ai_core_utilization_path, f"No ai_core_utilization.csv found in {self.prof_path}"
        FileChecker.check_csv_data_non_negative(ai_core_utilization_path[0], comparison_func=_is_non_negative,
                                                columns=["ub_read_bw(GB/s)", "ub_write_bw(GB/s)",
                                                         "l1_read_bw(GB/s)", "l1_write_bw(GB/s)", "l2_read_bw(GB/s)",
                                                         "l2_write_bw(GB/s)", "main_mem_read_bw(GB/s)",
                                                         "main_mem_write_bw(GB/s)"])

    @abstractmethod
    def check_ai_vector_core_utilization_data(self):
        ai_vector_core_utilization_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                                    f"ai_vector_core_utilization*.csv")
        assert ai_vector_core_utilization_path, f"No ai_vector_core_utilization.csv found in {self.prof_path}"
        FileChecker.check_csv_data_non_negative(ai_vector_core_utilization_path[0], comparison_func=_is_non_negative,
                                                columns=["ub_read_bw(GB/s)", "ub_write_bw(GB/s)",
                                                         "l1_read_bw(GB/s)", "l1_write_bw(GB/s)", "l2_read_bw(GB/s)",
                                                         "l2_write_bw(GB/s)", "main_mem_read_bw(GB/s)",
                                                         "main_mem_write_bw(GB/s)"])

    @abstractmethod
    def check_api_statistic_data(self):
        api_statistic_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                       f"api_statistic*.csv")
        assert api_statistic_path, f"No api_statistic.csv found in {self.prof_path}"
        FileChecker.check_csv_items(api_statistic_path[0], {"Level": ["acl", "runtime", "node"],
                                                            "Device_id": ["host"]},
                                    fuzzy_match=False)
        FileChecker.check_csv_items(api_statistic_path[0], {"API Name": ["Add*", "Cast*", "Relu*"]},
                                    fuzzy_match=True)
        FileChecker.check_csv_data_non_negative(api_statistic_path[0], comparison_func=_is_non_negative,
                                                columns=["Time(us)", "Count", "Avg(us)",
                                                         "Min(us)", "Max(us)",
                                                         "Variance"])

    @abstractmethod
    def check_hbm_data(self):
        hbm_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                             f"hbm*.csv")
        assert hbm_path, f"No hbm.csv found in {self.prof_path}"
        FileChecker.check_csv_data_non_negative(hbm_path[0], comparison_func=_is_non_negative,
                                                columns=["Read(MB/s)", "Write(MB/s)"])

    @abstractmethod
    def check_l2_cache_data(self):
        l2_cache_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                  f"l2_cache*.csv")
        assert l2_cache_path, f"No l2_cache.csv found in {self.prof_path}"
        FileChecker.check_csv_data_non_negative(l2_cache_path[0], comparison_func=_is_non_negative,
                                                columns=["Stream Id", "Task Id", "Victim Rate"])
        FileChecker.check_csv_items(l2_cache_path[0], {"Op Name": ["Conv2D*", "Add*", "TransData*"]},
                                    fuzzy_match=True)

    @abstractmethod
    def check_llc_read_write_data(self):
        llc_read_write_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                        f"llc_read_write*.csv")
        assert llc_read_write_path, f"No llc_read_write.csv found in {self.prof_path}"
        FileChecker.check_csv_data_non_negative(llc_read_write_path[0], comparison_func=_is_non_negative,
                                                columns=["Hit Rate(%)", "Throughput(MB/s)"])

    @abstractmethod
    def check_npu_mem_data(self):
        npu_mem_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                 f"npu_mem*.csv")
        assert npu_mem_path, f"No npu_mem.csv found in {self.prof_path}"
        FileChecker.check_csv_data_non_negative(npu_mem_path[0], comparison_func=_is_non_negative,
                                                columns=["ddr(KB)", "hbm(KB)", "memory(KB)", "timestamp(us)"])

    @abstractmethod
    def check_npu_module_mem_data(self):
        npu_module_mem_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                        f"npu_module_mem*.csv")
        assert npu_module_mem_path, f"No npu_mem.csv found in {self.prof_path}"
        FileChecker.check_csv_data_non_negative(npu_module_mem_path[0], comparison_func=_is_non_negative,
                                                columns=["Timestamp(us)", "Total Reserved(KB)"])
        FileChecker.check_csv_items(npu_module_mem_path[0], {"Component": ["APP"]},
                                    fuzzy_match=False)

    @abstractmethod
    def check_soc_pmu_data(self):
        soc_pmu_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                 f"soc_pmu*.csv")
        assert soc_pmu_path, f"No soc_pmu.csv found in {self.prof_path}"
        FileChecker.check_csv_data_non_negative(soc_pmu_path[0], comparison_func=_is_non_negative,
                                                columns=["Stream Id", "Task Id", "TLB Hit Rate", "TLB Miss Rate"])
        FileChecker.check_csv_items(soc_pmu_path[0], {"Op Name": ["Conv2D*", "*TransData*", "*Add*"]},
                                    fuzzy_match=True)

    @abstractmethod
    def check_task_time_data(self):
        task_time_path = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_output/"
                                   f"task_time*.csv")
        assert task_time_path, f"No task_time.csv found in {self.prof_path}"
        FileChecker.check_csv_data_non_negative(task_time_path[0], comparison_func=_is_non_negative,
                                                columns=["Stream Id", "Task Id", "task_time(us)", "task_start(us)",
                                                         "task_stop(us)"])
        FileChecker.check_csv_items(task_time_path[0], {"kernel_name": ["Conv2D*", "*TransData*", "*Add*"]},
                                    fuzzy_match=True)
        FileChecker.check_csv_items(task_time_path[0], {"kernel_type": ["AI_VECTOR_CORE", "AI_CORE",
                                                                        "MIX_AIV"]}, fuzzy_match=False)

    @abstractmethod
    def check_other_data(self):
        pass

    def check_msprof_log(self):
        msprof_log_paths = glob.glob(f"{self.prof_path}/PROF_*/mindstudio_profiler_log/*.log")
        assert msprof_log_paths, f"No msprof log file found in {self.prof_path}"
        for msprof_log_path in msprof_log_paths:
            FileChecker.check_file_for_keyword(msprof_log_path, "error")

    def check_msprof_db_file(self):
        db_path = glob.glob(f"{self.prof_path}/PROF_*/msprof_*.db")
        assert db_path, f"No db file found in {self.prof_path}"
        FileChecker.check_file_exists(db_path[0])
        for table_name in self.expect_db_tables:
            # 1. Check table exist
            FileChecker.check_db_table_exist(db_path[0], table_name)
            # 2. Check table fields
            res_table_fields = DBManager.fetch_all_field_name_in_table(db_path[0], table_name)
            assert res_table_fields == TableFields.get_fields(table_name), \
                (f"Fields for table '{table_name}' do not match. Expected: {TableFields.get_fields(table_name)}, "
                 f"Actual: {res_table_fields}")
