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

import glob
import sys

import logging

from utils.db_check import DBManager
from utils.file_check import FileChecker
from utils.table_fields import TableFields

logging.basicConfig(level=logging.INFO,
                    format='\n%(asctime)s %(filename)s [line:%(lineno)d] [%(levelname)s] %(message)s')


def test_ascend_msprof_aic_mode_sample_based_read_memory(prof_path):
    # 1. Verify the prof data result of the text type
    _check_text_files(prof_path)
    # 2. Verify the prof data result of the db type
    _check_db_files(prof_path)


def _check_text_files(prof_path):
    # 1. trace_view.json
    msprof_json_path = glob.glob(f"{prof_path}/PROF_*/mindstudio_profiler_output/msprof_*.json")
    assert msprof_json_path, f"No msprof.json found in {prof_path}"
    FileChecker.check_timeline_values(msprof_json_path[0], "cat", ["HostToDevice", ], fuzzy_match=True)
    FileChecker.check_timeline_values(msprof_json_path[0], "name", ["aclnnMatmul_*"], fuzzy_match=True)
    # 2. op_summary.json
    op_summary_path = glob.glob(f"{prof_path}/PROF_*/mindstudio_profiler_output/op_summary_*.csv")
    assert op_summary_path, f"No op_summary.json found in {prof_path}"
    FileChecker.check_csv_items(op_summary_path[0], {"Op Name": ["aclnnMatmul_*"]})
    FileChecker.check_csv_data_non_negative(op_summary_path[0], comparison_func=_is_non_negative,
                                            columns=["Task Duration(us)", "Task Wait Time(us)", "Task Start Time(us)"])
    # 3. profiler.log
    msprof_log_paths = glob.glob(f"{prof_path}/PROF_*/mindstudio_profiler_log/*.log")
    assert msprof_log_paths, f"No msprof log file found in {prof_path}"
    for msprof_log_path in msprof_log_paths:
        FileChecker.check_file_for_keyword(msprof_log_path, "error")


def _check_db_files(prof_path):
    db_path = glob.glob(f"{prof_path}/PROF_*/msprof_*.db")
    assert db_path, f"No db file found in {prof_path}"
    FileChecker.check_file_exists(db_path[0])
    expect_tables = ["AICORE_FREQ", "CANN_API", "COMPUTE_TASK_INFO", "ENUM_API_TYPE", "ENUM_HCCL_DATA_TYPE",
                     "ENUM_HCCL_LINK_TYPE", "ENUM_HCCL_RDMA_TYPE", "ENUM_HCCL_TRANSPORT_TYPE",
                     "ENUM_MEMCPY_OPERATION", "ENUM_MODULE", "ENUM_MSTX_EVENT_TYPE", "HOST_INFO", "META_DATA",
                     "NPU_INFO", "RANK_DEVICE_MAP", "SESSION_TIME_INFO", "STRING_IDS", "TASK", "TASK_PMU_INFO"]
    for table_name in expect_tables:
        # 1. Check table exist
        FileChecker.check_db_table_exist(db_path[0], table_name)
        # 2. Check table fields
        res_table_fields = DBManager.fetch_all_field_name_in_table(db_path[0], table_name)
        assert res_table_fields == TableFields.get_fields(table_name), \
            (f"Fields for table '{table_name}' do not match. Expected: {TableFields.get_fields(table_name)}, "
             f"Actual: {res_table_fields}")


def _is_non_negative(value):
    """ Check if a given value is non-negative (i.e., greater than or equal to zero)."""
    return float(value) >= 0.0


if __name__ == '__main__':
    path = sys.argv[1]
    test_ascend_msprof_task_time_l1(path)
