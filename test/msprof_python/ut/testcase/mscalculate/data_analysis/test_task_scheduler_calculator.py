#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from mscalculate.data_analysis.task_scheduler_calculator import TaskSchedulerCalculator
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.chip_model import ChipModel
from constant.constant import CONFIG

from sqlite.db_manager import DBManager
from analyzer.scene_base.profiling_scene import ProfilingScene

file_list = {}
NAMESPACE = 'mscalculate.data_analysis.task_scheduler_calculator'


class TestTaskSchedulerCalculator(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.TaskSchedulerCalculator.process'):
            key = TaskSchedulerCalculator(file_list, CONFIG)
            key.ms_run()

    def test_process(self):
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        with mock.patch(NAMESPACE + '.TaskSchedulerCalculator.generate_report_data'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.process()
        with mock.patch(NAMESPACE + '.logging.error'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.process()

    def test_create_task_time(self):
        device = 123
        iter_time_range = [1, 1]
        create_sql = "create table IF NOT EXISTS TimeLine (replayid INTEGER,tasktype INTEGER," \
                     "task_id INTEGER,stream_id INTEGER,taskstate INTEGER,timestamp INTEGER," \
                     "thread INTEGER,device_id INTEGER,mode INTEGER)"
        data = ((0, 2, 3, 0, 1, 173202761738, 1752, 0, 1),)
        insert_sql = "insert into {0} values ({value})".format("TimeLine", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        res[1].execute("CREATE TABLE if not exists TaskTime(replayid INTEGER,device_id INTEGER,"
                       "api INTEGER,apirowid INTEGER,tasktype INTEGER,task_id INTEGER,"
                       "stream_id INTEGER,waittime TEXT,pendingtime Text,runtime TEXT,"
                       "completetime TEXT,index_id INTEGER,model_id INTEGER)")
        runtime_conn = res[0]
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.logging.warning'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.create_task_time(runtime_conn, device, iter_time_range)
        db_manager.destroy(res)
        res = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        runtime_conn = res[0]
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=create_sql), \
                mock.patch(NAMESPACE + '.multi_calculate_task_cost_time',
                           return_value=[(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)]):
            key = TaskSchedulerCalculator(file_list, {"result_dir": ""})
            ProfilingScene().init("")
            key.iter_range = IterationRange(1, 1, 1)
            key.create_task_time(runtime_conn, device, iter_time_range)
        db_manager.destroy(res)
        db_manager = DBManager()
        res = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        runtime_conn = res[0]
        with mock.patch(NAMESPACE + '.TaskSchedulerCalculator._get_timeline_data', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.create_task_time(runtime_conn, device, iter_time_range)
        res[1].execute("drop table TimeLine")
        res[0].commit()
        db_manager.destroy(res)

    def test_update(self):
        api_data = [(1, 2, 3, '7,8', '0,0'), ]
        key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
        result = key.update(api_data)
        self.assertEqual(result, [(1, 2, 3, '7', '0'), (1, 2, 3, '8', '0')])

    def test_update_timeline_api(self):
        create_sql = "create table IF NOT EXISTS TaskTime (replayid INTEGER,device_id INTEGER," \
                     "api INTEGER,apirowid INTEGER,tasktype INTEGER,task_id INTEGER,stream_id INTEGER," \
                     "waittime TEXT,pendingtime Text,runtime TEXT,completetime TEXT,index_id INTEGER," \
                     "model_id INTEGER)"
        create_sql_1 = "create table IF NOT EXISTS ApiCall (replayid INTEGER,entry_time INTEGER," \
                       "exit_time INTEGER,api INTEGER,retcode INTEGER,thread INTEGER,device_id INTEGER," \
                       "stream_id INTEGER,tasknum INTEGER,task_id TEXT,detail TEXT,mode INTEGER)"
        data = ((0, 0, 4, 430, 4, 2, 5, 0, 0, 118562436328.0, 118562438671.0, 1, 1),)
        data_1 = ((0, 117960819118, 117962052296, 25, 0, 2246, 0, 65535, 2, 0, 0, 0),)
        insert_sql = "insert into {0} values ({value})".format("TaskTime", value="?," * (len(data[0]) - 1) + "?")
        insert_sql_1 = "insert into {0} values ({value})".format("ApiCall", value="?," * (len(data_1[0]) - 1) + "?")
        db_manager = DBManager()
        db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        res = db_manager.create_table("runtime.db", create_sql_1, insert_sql_1, data_1)
        runtime_conn = res[0]
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.logging.warning'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.update_timeline_api(runtime_conn)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.update_timeline_api(runtime_conn)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.TaskSchedulerCalculator.update', side_effect=TypeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.update_timeline_api(runtime_conn)
        res[1].execute("drop table TaskTime")
        res[1].execute("drop table ApiCall")
        res[0].commit()
        db_manager.destroy(res)

    def test_pre_mini_task_data(self):
        project_path = 'home\\runtime'
        device_id = 123
        iter_time_range = [1, 1]
        db_manager = DBManager()
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key._TaskSchedulerCalculator__pre_mini_task_data(project_path, device_id, iter_time_range)
        res = db_manager.create_table("runtime.db")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(res[0], res[1])), \
                mock.patch(NAMESPACE + '.TaskSchedulerCalculator.create_task_time'), \
                mock.patch(NAMESPACE + '.TaskSchedulerCalculator.update_timeline_api'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key._TaskSchedulerCalculator__pre_mini_task_data(project_path, device_id, iter_time_range)
        with mock.patch(NAMESPACE + '.TaskSchedulerCalculator.create_task_time', side_effect=TypeError), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key._TaskSchedulerCalculator__pre_mini_task_data(project_path, device_id, iter_time_range)
        db_manager.destroy(res)

    def test_insert_report_data(self):
        project_path = 'home\\runtime'
        device_id = 123
        create_sql = "create table IF NOT EXISTS TaskTime (replayid INTEGER,device_id INTEGER," \
                     "api INTEGER,apirowid INTEGER,tasktype INTEGER,task_id INTEGER,stream_id INTEGER," \
                     "waittime TEXT,pendingtime Text,runtime TEXT,completetime TEXT,index_id INTEGER," \
                     "model_id INTEGER)"
        create_sql_1 = "create table IF NOT EXISTS ReportTask (timeratio REAL,time REAL,count INTEGER," \
                       "avg REAL,min REAL,max REAL,waiting REAL,running REAL,pending REAL,type TEXT," \
                       "api TEXT,task_id INTEGER,stream_id INTEGER,device_id INTEGE)"
        data = ((0, 0, 4, 430, 4, 2, 5, 0, 0, 118562436328.0, 118562438671.0, 1, 1),)
        data_1 = ((42.1347, 9044.64, 1, 9044.64, 9044.64, 9044.64, 59.268, 9044.64, 0,
                   'model execute task', 'ModelExecute', 2, 3, 0),)
        insert_sql = "insert into {0} values ({value})".format("TaskTime", value="?," * (len(data[0]) - 1) + "?")
        insert_sql_1 = "insert into {0} values ({value})".format("ReportTask", value="?," * (len(data_1[0]) - 1) + "?")

        db_manager = DBManager()
        db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        res = db_manager.create_table("runtime.db", create_sql_1, insert_sql_1, data_1)
        with mock.patch(NAMESPACE + '.PathManager.get_sql_dir'), \
                mock.patch('os.path.join', return_value="runtime.db"):
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(False, False)), \
                    mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
                key.insert_report_data(project_path, device_id)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=res), \
                    mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.drop_table'), \
                    mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=create_sql_1):
                with mock.patch(NAMESPACE + '.calculate_task_schedule_data', return_value=False), \
                        mock.patch(NAMESPACE + '.logging.info'):
                    key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
                    key.insert_report_data(project_path, device_id)
        res = db_manager.create_table("runtime.db")
        with mock.patch(NAMESPACE + '.PathManager.get_sql_dir'), \
                mock.patch('os.path.join', return_value="runtime.db"):
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=res), \
                    mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.drop_table'), \
                    mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=create_sql_1), \
                    mock.patch(NAMESPACE + '.calculate_task_schedule_data', return_value=data_1), \
                    mock.patch(NAMESPACE + '.logging.info'):
                key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
                key.insert_report_data(project_path, device_id)

        res = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        res[1].execute("drop table TaskTime")
        res[1].execute("drop table ReportTask")
        res[0].commit()
        res[0].commit()
        db_manager.destroy(res)

    def test_generate_report_data_1(self):
        setattr(InfoConfReader(), "_info_json", {"devices": '123'})
        with mock.patch(NAMESPACE + '.MsprofIteration.get_step_syscnt_range_by_iter_range',
                        return_value=False):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.generate_report_data()
        with mock.patch(NAMESPACE + '.MsprofIteration.get_step_syscnt_range_by_iter_range',
                        return_value=True), \
                mock.patch(NAMESPACE + '.TaskSchedulerCalculator._TaskSchedulerCalculator__pre_mini_task_data'), \
                mock.patch(NAMESPACE + '.TaskSchedulerCalculator.insert_report_data'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.generate_report_data()

    def test_generate_report_data_2(self):
        setattr(InfoConfReader(), "_info_json", {"devices": None})
        with mock.patch(NAMESPACE + '.logging.error'):
            key = TaskSchedulerCalculator(file_list, {"result_dir": 123})
            key.generate_report_data()

    def test_generate_report_data(self):
        setattr(InfoConfReader(), "_info_json", {'devices': '0'})
        with mock.patch(NAMESPACE + '.MsprofIteration.get_step_syscnt_range_by_iter_range',
                        return_value=1), \
                mock.patch(NAMESPACE + '.TaskSchedulerCalculator._TaskSchedulerCalculator__pre_mini_task_data'), \
                mock.patch(NAMESPACE + '.TaskSchedulerCalculator.insert_report_data'):
            check = TaskSchedulerCalculator(file_list, CONFIG)
            check.generate_report_data()


if __name__ == '__main__':
    unittest.main()
