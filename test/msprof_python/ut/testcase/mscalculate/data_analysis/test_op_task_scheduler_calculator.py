#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.

import sqlite3
import unittest
from unittest import mock

from mscalculate.data_analysis.op_task_scheduler_calculator import OpTaskSchedulerCalculator
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel
from constant.constant import CONFIG

from sqlite.db_manager import DBManager

file_list = {}
NAMESPACE = 'mscalculate.data_analysis.op_task_scheduler_calculator'


class TestCalculateOpTaskScheduler(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.OpTaskSchedulerCalculator.process'):
            key = OpTaskSchedulerCalculator(file_list, CONFIG)
            key.ms_run()

    def test_process(self):
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        with mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                        return_value="single_op"):
            with mock.patch(NAMESPACE + '.OpTaskSchedulerCalculator.op_generate_report_data'):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                check.process()
        with mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                           return_value="single_op"):
            with mock.patch(NAMESPACE + '.OpTaskSchedulerCalculator.op_generate_report_data', side_effect=OSError), \
                    mock.patch(NAMESPACE + '.logging.error'):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                check.process()

    def test_update(self):
        api_data = [(25, 1, 65535, '0,0', '0,0'), (6, 2, 4, '0,0', '0,0'), (6, 6, 5, '0,0', '0,0')]
        expect_result = [(25, 1, 65535, '0', '0'),
                         (25, 1, 65535, '0', '0'),
                         (6, 2, 4, '0', '0'),
                         (6, 2, 4, '0', '0'),
                         (6, 6, 5, '0', '0'),
                         (6, 6, 5, '0', '0')]
        check = OpTaskSchedulerCalculator(file_list, CONFIG)
        result = getattr(check, "_update")(api_data)
        self.assertEqual(result, expect_result)

    def test_op_create_task_time(self):
        create_sql = "CREATE TABLE IF NOT EXISTS TimeLine (replayid INTEGER,tasktype INTEGER,task_id INTEGER," \
                     "stream_id INTEGER,taskstate INTEGER,timestamp INTEGER,thread INTEGER,device_id INTEGER," \
                     "mode INTEGER)"
        insert_sql = "insert into {} values (?,?,?,?,?,?,?,?,?)".format('TimeLine')
        data = ((1, 1, 2, 3, 4, 5, 6, 0, 8),)
        db_manager = DBManager()
        res = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        res[1].execute("CREATE TABLE IF NOT EXISTS TaskTime (id INTEGER, name INTEGER)")
        runtime_conn = res[0]
        device = 0
        cal_task_data = [(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10), (3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)]
        with mock.patch(NAMESPACE + '.logging.info'):
            with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                result = check.op_create_task_time(runtime_conn, device)
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch(NAMESPACE + '.multi_calculate_task_cost_time', return_value=cal_task_data):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                check.index_id = 1
                check.op_create_task_time(runtime_conn, device)
            with mock.patch(NAMESPACE + '.OpTaskSchedulerCalculator._get_timeline_data', side_effect=SystemError):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                check.index_id = 1
                check.op_create_task_time(runtime_conn, device)
        res[0].execute("drop table TimeLine")
        res[0].execute("drop table TaskTime")
        db_manager.destroy(res)

    def test_op_update_timeline_api(self):
        create_sql = "CREATE TABLE IF NOT EXISTS ApiCall(replayid INTEGER,entry_time INTEGER,exit_time INTEGER," \
                     "api INTEGER,retcode INTEGER,thread INTEGER,device_id INTEGER,stream_id INTEGER," \
                     "tasknum INTEGER,task_id TEXT,detail TEXT,mode INTEGER)"
        insert_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?,?)".format('ApiCall')
        data = ((0, 1, 2, 3, 4, 5, 6, 0, 8, 9, 10, 11),)
        create_task_time_sql = "CREATE TABLE IF NOT EXISTS TaskTime(replayid INTEGER,device_id INTEGER," \
                               "api INTEGER,apirowid INTEGER,tasktype INTEGER,task_id INTEGER,stream_id INTEGER," \
                               "waittime TEXT,pendingtime Text,runtime TEXT,completetime TEXT,index_id INTEGER," \
                               "model_id INTEGER)"
        insert_task_time_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?,?,?)".format('TaskTime')
        task_time_data = ((0, 1, 2, 3, 4, 5, 6, 0, 8, 9, 10, 11, 12),)
        db_manager = DBManager()
        db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        res = db_manager.create_table("runtime.db", create_task_time_sql, insert_task_time_sql, task_time_data)
        runtime_conn = res[0]
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.logging.warning'):
            with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                result = check.op_update_timeline_api(runtime_conn)
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                check.op_update_timeline_api(runtime_conn)
                with mock.patch(NAMESPACE + '.OpTaskSchedulerCalculator._update', side_effect=OSError):
                    check = OpTaskSchedulerCalculator(file_list, CONFIG)
                    check.op_update_timeline_api(runtime_conn)
        res[1].execute("drop table ApiCall")
        res[1].execute("drop table TaskTime")
        db_manager.destroy(res)

    def test_op_pre_mini_task_data(self):
        project_path = 'test\\test'
        device_id = 0
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True):
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(False, False)):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                result = check.op_pre_mini_task_data(project_path, device_id)
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(True, True)), \
                    mock.patch(NAMESPACE + '.OpTaskSchedulerCalculator.op_create_task_time'), \
                    mock.patch(NAMESPACE + '.OpTaskSchedulerCalculator.op_update_timeline_api'):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                check.op_pre_mini_task_data(project_path, device_id)

    def test_op_insert_report_data(self):
        project_path = 'test\\test'
        device_id = 0
        report_data = [(1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 8, 9, 8, 2)]
        sql = 'CREATE TABLE IF NOT EXISTS ReportTask(timeratio REAL,time REAL,count INTEGER,avg REAL,min REAL,' \
              'max REAL,waiting REAL,running REAL,pending REAL,type TEXT,api TEXT,task_id INTEGER,stream_id INTEGER,' \
              'device_id INTEGER)'
        db_manager = DBManager()
        res = db_manager.create_table("runtime.db")
        res[1].execute("CREATE TABLE IF NOT EXISTS ReportTask (id INTEGER, name INTEGER)")
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=""), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir'), \
                mock.patch('os.path.join'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(False, False)):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                result = check.op_insert_report_data(project_path, device_id)
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=res), \
                    mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                    mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True):
                with mock.patch(NAMESPACE + '.calculate_task_schedule_data', return_value=False):
                    check = OpTaskSchedulerCalculator(file_list, CONFIG)
                    result = check.op_insert_report_data(project_path, device_id)
                self.assertEqual(result, None)
                with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=sql), \
                        mock.patch(NAMESPACE + '.calculate_task_schedule_data', return_value=report_data):
                    check = OpTaskSchedulerCalculator(file_list, CONFIG)
                    check.op_insert_report_data(project_path, device_id)
        res[1].execute("drop TABLE ReportTask")
        db_manager.destroy(res)

    def test_op_generate_report_data(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True):
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                result = check.op_generate_report_data()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False), \
                    mock.patch(NAMESPACE + '.OpTaskSchedulerCalculator.op_pre_mini_task_data'), \
                    mock.patch(NAMESPACE + '.OpTaskSchedulerCalculator.op_insert_report_data'):
                check = OpTaskSchedulerCalculator(file_list, CONFIG)
                check.op_generate_report_data()




if __name__ == '__main__':
    unittest.main()
