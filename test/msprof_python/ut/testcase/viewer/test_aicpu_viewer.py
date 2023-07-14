import sqlite3
import struct
import unittest
from collections import OrderedDict
from unittest import mock

from constant.constant import ITER_RANGE
from profiling_bean.db_dto.step_trace_dto import IterationRange
from sqlite.db_manager import DBOpen
from viewer.aicpu_viewer import ParseAiCpuData
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant


sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'viewer.aicpu_viewer'


def test_analysis_aicpu():
    project_path = 'home\\project'
    headers = ["Timestamp", "Node", "Compute_time(us)", "Memcpy_time(us)", "Task_time(us)",
               "Dispatch_time(us)", "Total_time(us)", "Stream ID", "Task ID"]
    with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='ai_cpu.db'), \
            mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(False, False)):
            result = ParseAiCpuData.analysis_aicpu(project_path, ITER_RANGE)
            unittest.TestCase().assertEqual(result, (headers, []))
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.ParseAiCpuData.get_ai_cpu_data', return_value=[]):
            result = ParseAiCpuData.analysis_aicpu(project_path, ITER_RANGE)
            unittest.TestCase().assertEqual(result, (headers, []))


def test_get_ai_cpu_sql():
    with DBOpen('test.db') as db_open:
        with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
            ProfilingScene().init(" ")
            ProfilingScene()._scene = Constant.SINGLE_OP
            result = ParseAiCpuData.get_ai_cpu_data("", ITER_RANGE, db_open.db_conn)
            unittest.TestCase().assertEqual(result, [])

        with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
            ProfilingScene().init(" ")
            ProfilingScene()._scene = Constant.STEP_INFO
            result = ParseAiCpuData.get_ai_cpu_data("", ITER_RANGE, db_open.db_conn)
            unittest.TestCase().assertEqual(result, [])

        with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
            ProfilingScene().init(" ")
            ProfilingScene()._scene = Constant.STEP_INFO
            result = ParseAiCpuData.get_ai_cpu_data("", ITER_RANGE, db_open.db_conn)
            unittest.TestCase().assertEqual(result, [])


def test_get_aicpu_data_missing_ge_data():
    with DBOpen('test.db') as db_open:
        with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
            ProfilingScene().init(" ")
            ProfilingScene()._scene = Constant.STEP_INFO
            result = ParseAiCpuData._get_aicpu_data_missing_ge_data(db_open.db_conn, ITER_RANGE, "")
            unittest.TestCase().assertEqual(result, [])


def test_get_aicpu_data_with_ge_data():
    with DBOpen('test.db') as db_open:
        with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
            ProfilingScene().init(" ")
            ProfilingScene()._scene = Constant.STEP_INFO
            result = ParseAiCpuData._get_aicpu_data_with_ge_data(db_open.db_conn, ITER_RANGE, "")
            unittest.TestCase().assertEqual(result, [])


if __name__ == '__main__':
    unittest.main()
