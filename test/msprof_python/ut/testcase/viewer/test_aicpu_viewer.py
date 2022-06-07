import sqlite3
import struct
import unittest
from collections import OrderedDict
from unittest import mock

from sqlite.db_manager import DBOpen
from viewer.aicpu_viewer import ParseAiCpuData
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant


sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'viewer.aicpu_viewer'


def test_analysis_aicpu():
    project_path = 'home\\project'
    headers = ["Timestamp", "Node", "Compute_time(ms)", "Memcpy_time(ms)", "Task_time(ms)",
               "Dispatch_time(ms)", "Total_time(ms)"]
    with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='ai_cpu.db'), \
            mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(False, False)):
            result = ParseAiCpuData.analysis_aicpu(project_path, 0, 1)
            unittest.TestCase().assertEqual(result, (headers, []))
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.ParseAiCpuData.get_ai_cpu_data', return_value=[]):
            result = ParseAiCpuData.analysis_aicpu(project_path, 0, 1)
            unittest.TestCase().assertEqual(result, (headers, []))


def test_get_ai_cpu_sql():
    with DBOpen('test.db') as db_open:
        with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
            ProfilingScene().init(" ")
            ProfilingScene()._scene = Constant.SINGLE_OP
            result = ParseAiCpuData.get_ai_cpu_data("", 1, 0, db_open.db_conn)
            unittest.TestCase().assertEqual(result, [])

        with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iter_dict_with_index_and_model', return_value={}), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
            ProfilingScene().init(" ")
            ProfilingScene()._scene = Constant.STEP_INFO
            result = ParseAiCpuData.get_ai_cpu_data("", 1, 0, db_open.db_conn)
            unittest.TestCase().assertEqual(result, [])

        with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
            ProfilingScene().init(" ")
            ProfilingScene()._scene = Constant.STEP_INFO
            result = ParseAiCpuData.get_ai_cpu_data("", 1, 0, db_open.db_conn)
            unittest.TestCase().assertEqual(result, [])


if __name__ == '__main__':
    unittest.main()
