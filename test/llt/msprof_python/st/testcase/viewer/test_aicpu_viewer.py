import sqlite3
import struct
import unittest
from collections import OrderedDict
from unittest import mock

from viewer.aicpu_viewer import ParseAiCpuData
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant

from sqlite.db_manager import DBManager

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'viewer.aicpu_viewer'


def test_analysis_aicpu():
    project_path = 'home\\project'
    iteration_id = '0'
    device_id = '123'
    sql = "select sys_start,node_name,compute_time,memcpy_time,task_time,dispatch_time," \
          "total_time from ai_cpu_datas where sys_start>0 and sys_start<3"
    headers = ["Timestamp", "Node", "Compute_time(ms)", "Memcpy_time(ms)", "Task_time(ms)",
               "Dispatch_time(ms)", "Total_time(ms)"]
    create_sql = "create table IF NOT EXISTS ai_cpu_datas (timestamp REAL,sys_start INTEGER," \
                 "sys_end INTEGER, node_name TEXT,compute_time REAL,memcpy_time REAL," \
                 "task_time REAL,dispatch_time REAL,total_time REAL)"
    db_manager = DBManager()
    res = db_manager.create_table("ai_cpu.db")
    res[1].execute(create_sql)
    with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='ai_cpu.db'), \
            mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(False, False)):
            result = ParseAiCpuData.analysis_aicpu(project_path, iteration_id, device_id)
            unittest.TestCase().assertEqual(result, (headers, []))
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(res[0], res[1])), \
                mock.patch(NAMESPACE + '.ParseAiCpuData.get_ai_cpu_sql', return_value=sql):
            result = ParseAiCpuData.analysis_aicpu(project_path, iteration_id, device_id)
            unittest.TestCase().assertEqual(result, (headers, []))


def test_get_ai_cpu_sql():
    sql_1 = "select sys_start,op_name,compute_time,memcpy_time,task_time,dispatch_time, total_time" \
            " from ai_cpu_datas join TaskInfo on " \
            "sys_start>=0.0 and sys_end<=0.001 and ai_cpu_datas.stream_id=TaskInfo.stream_id and" \
            " ai_cpu_datas.task_id=TaskInfo.task_id and ai_cpu_datas.batch_id=TaskInfo.batch_id and " \
            "TaskInfo.model_id=1 and (TaskInfo.index_id=0 or TaskInfo.index_id=0) and TaskInfo.task_type='AI_CPU'"
    sql_2 = "select sys_start,'N/A',compute_time,memcpy_time,task_time,dispatch_time,total_time from ai_cpu_datas " \
            "where sys_start>=0.0 and sys_end<=0.001"
    with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
          mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
        ProfilingScene().init(" ")
        ProfilingScene()._scene = Constant.STEP_INFO
        result = ParseAiCpuData.get_ai_cpu_sql("", 1, 0, "")
        unittest.TestCase().assertEqual(result, sql_1)

    with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=False), \
          mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]):
        ProfilingScene().init(" ")
        ProfilingScene()._scene = Constant.STEP_INFO
        result = ParseAiCpuData.get_ai_cpu_sql("", 1, 0, "")
        unittest.TestCase().assertEqual(result, sql_2)


if __name__ == '__main__':
    unittest.main()
