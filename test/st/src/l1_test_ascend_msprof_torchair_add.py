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
# MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import glob
import json
import os
import sys

import pandas as pd
from utils.db_check import DBManager
from utils.file_check import FileChecker
from utils.table_fields import TableFields

INPUT_SHAPES_COLUMN = "Input Shapes"
KERNEL_DETAILS_FILE = "kernel_details.csv"
OPERATOR_DETAILS_FILE = "operator_details.csv"
API_STATISTIC_FILE = "api_statistic.csv"
OP_STATISTIC_FILE = "op_statistic.csv"
STEP_TRACE_TIME_FILE = "step_trace_time.csv"
TASK_TIME_FILE = "task_time.csv"
TRACE_VIEW_FILE = "trace_view.json"
ANALYSIS_DB_FILE = "analysis.db"
ASCEND_PYTORCH_PROFILER_DB_FILE = "ascend_pytorch_profiler.db"


def _resolve_output_dir(prof_path: str) -> str:
    pt_dirs = glob.glob(os.path.join(prof_path, "*_pt"))
    if not pt_dirs:
        raise FileNotFoundError(f"No *_pt directory found under {prof_path}")

    output_dir = os.path.join(pt_dirs[0], "ASCEND_PROFILER_OUTPUT")
    if not os.path.isdir(output_dir):
        raise FileNotFoundError(f"No ASCEND_PROFILER_OUTPUT directory found under {pt_dirs[0]}")
    return output_dir


def _check_kernel_details(output_dir: str):
    kernel_details_path = os.path.join(output_dir, KERNEL_DETAILS_FILE)
    FileChecker.check_csv_headers(kernel_details_path, ["Name", "Type", INPUT_SHAPES_COLUMN])
    df = pd.read_csv(kernel_details_path)

    add_rows = df[
        df["Name"].fillna("").astype(str).str.contains("add", case=False, regex=False)
        | df["Type"].fillna("").astype(str).str.contains("add", case=False, regex=False)
    ]
    if add_rows.empty:
        raise AssertionError("No Add rows found in kernel_details.csv.")

    values = add_rows[INPUT_SHAPES_COLUMN].fillna("").astype(str).str.strip().str.upper()
    if values.isin({"", "N/A", "NA", "NAN"}).all():
        raise AssertionError("Expected Add 'Input Shapes' to contain non-N/A values in kernel_details.csv.")


def _check_api_statistic(output_dir: str):
    api_statistic_path = os.path.join(output_dir, API_STATISTIC_FILE)
    FileChecker.check_csv_headers(
        api_statistic_path,
        ["Device_id", "Level", "API Name", "Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"],
    )
    FileChecker.check_csv_items(
        api_statistic_path,
        {"API Name": ["*aclmdlRIExecuteAsync*", "*aclnnForeachCopy*", "*aclrtLaunchKernelWithHostArgs*"]},
    )


def _check_op_statistic(output_dir: str):
    op_statistic_path = os.path.join(output_dir, OP_STATISTIC_FILE)
    FileChecker.check_csv_headers(
        op_statistic_path,
        ["Device_id", "OP Type", "Core Type", "Count", "Total Time(us)", "Min Time(us)", "Avg Time(us)", "Max Time(us)", "Ratio(%)"],
    )
    FileChecker.check_csv_items(op_statistic_path, {"OP Type": ["*Add*"]})


def _check_operator_details(output_dir: str):
    operator_details_path = os.path.join(output_dir, OPERATOR_DETAILS_FILE)
    FileChecker.check_file_exists(operator_details_path)
    FileChecker.check_csv_headers(
        operator_details_path,
        [
            "Name",
            "Input Shapes",
            "Host Self Duration(us)",
            "Host Total Duration(us)",
            "Device Self Duration(us)",
            "Device Total Duration(us)",
        ],
    )
    FileChecker.check_csv_items(
        operator_details_path,
        {"Name": ["*aclnnForeachCopy*", "*acl_graph_replay*"]},
    )


def _check_step_trace_time(output_dir: str):
    step_trace_time_path = os.path.join(output_dir, STEP_TRACE_TIME_FILE)
    FileChecker.check_csv_headers(
        step_trace_time_path,
        [
            "Device_id",
            "Step",
            "Computing",
            "Communication(Not Overlapped)",
            "Overlapped",
            "Communication",
            "Free",
            "Stage",
            "Bubble",
            "Communication(Not Overlapped and Exclude Receive)",
            "Preparing",
        ],
    )
    df = pd.read_csv(step_trace_time_path)
    if df.empty:
        raise AssertionError("step_trace_time.csv is empty.")


def _check_task_time(output_dir: str):
    task_time_path = os.path.join(output_dir, TASK_TIME_FILE)
    FileChecker.check_csv_headers(
        task_time_path,
        ["Device_id", "kernel_name", "kernel_type", "stream_id", "task_id", "task_time(us)", "task_start(us)", "task_stop(us)"],
    )
    FileChecker.check_csv_items(task_time_path, {"kernel_name": ["*ForeachCopy*", "*Add*"]})


def _check_trace_view(output_dir: str):
    trace_view_path = os.path.join(output_dir, TRACE_VIEW_FILE)
    FileChecker.check_file_exists(trace_view_path)
    FileChecker.check_timeline_values(trace_view_path, "name", ["*aclnnForeachCopy*", "*aclnnAdd_AddAiCore_Add*"])


def _check_table_columns(db_path: str, table_name: str, expected_columns):
    actual_columns = set(DBManager.fetch_all_field_name_in_table(db_path, table_name))
    missing_columns = set(expected_columns) - actual_columns
    if missing_columns:
        raise AssertionError(f"Missing columns in table '{table_name}': {sorted(missing_columns)}")


def _check_analysis_db(output_dir: str):
    analysis_db_path = os.path.join(output_dir, ANALYSIS_DB_FILE)
    tables = DBManager.fetch_all_table_names(analysis_db_path)
    if tables != ["StepTraceTime"]:
        raise AssertionError(f"Unexpected tables in analysis.db: {tables}")

    _check_table_columns(
        analysis_db_path,
        "StepTraceTime",
        [
            "deviceId",
            "step",
            "computing",
            "communication_not_overlapped",
            "overlapped",
            "communication",
            "free",
            "stage",
        ],
    )
    if DBManager.fetch_table_row_count(analysis_db_path, "StepTraceTime") <= 0:
        raise AssertionError("StepTraceTime table in analysis.db is empty.")


def _check_ascend_pytorch_profiler_db(output_dir: str):
    profiler_db_path = os.path.join(output_dir, ASCEND_PYTORCH_PROFILER_DB_FILE)
    tables = DBManager.fetch_all_table_names(profiler_db_path)
    expected_tables = [
        "CANN_API",
        "COMPUTE_TASK_INFO",
        "META_DATA",
        "NPU_INFO",
        "PYTORCH_API",
        "STEP_TIME",
        "TASK",
    ]
    missing_tables = set(expected_tables) - set(tables)
    if missing_tables:
        raise AssertionError(f"Missing expected tables in ascend_pytorch_profiler.db: {sorted(missing_tables)}")

    for table_name in expected_tables:
        expected_columns = TableFields.get_fields(table_name)
        if expected_columns is None:
            raise AssertionError(f"No table field definition found for '{table_name}' in TableFields.")
        _check_table_columns(profiler_db_path, table_name, expected_columns)

    for table_name in ["CANN_API", "PYTORCH_API", "STEP_TIME", "TASK"]:
        if DBManager.fetch_table_row_count(profiler_db_path, table_name) <= 0:
            raise AssertionError(f"Table '{table_name}' in ascend_pytorch_profiler.db is empty.")


def test_ascend_msprof_torchair_add(prof_path):
    output_dir = _resolve_output_dir(prof_path)
    _check_kernel_details(output_dir)
    _check_api_statistic(output_dir)
    _check_op_statistic(output_dir)
    _check_operator_details(output_dir)
    _check_step_trace_time(output_dir)
    _check_task_time(output_dir)
    _check_trace_view(output_dir)
    _check_analysis_db(output_dir)
    _check_ascend_pytorch_profiler_db(output_dir)


if __name__ == "__main__":
    test_ascend_msprof_torchair_add(sys.argv[1])
