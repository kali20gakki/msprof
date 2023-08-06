#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
import os
import unittest
from unittest import mock
import pytest

from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from mscalculate.hccl_calculator import HcclCalculator
from profiling_bean.db_dto.hccl_dto import HcclDto
from sqlite.db_manager import DBManager
from constant.constant import CONFIG


NAMESPACE = 'mscalculate.hccl_calculator'


class TestHcclCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_HcclCalculator')

    @staticmethod
    def construct_hccl_dto(op_name, timestamp=123456):
        hccl_data = HcclDto()
        hccl_data.op_name, hccl_data.iteration, hccl_data.first_timestamp, hccl_data.is_dynamic, \
        hccl_data.model_id, hccl_data.index_id, hccl_data.task_type, hccl_data.op_type = \
            (op_name, 1, timestamp, 0, 1, 1, "HCCL", "HCCL_OP_TYPE")
        return hccl_data

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_calculate(self):
        with mock.patch(NAMESPACE + ".HcclCalculator._get_hccl_data",
                        return_value=[self.construct_hccl_dto("hccl_op")]), \
                mock.patch(NAMESPACE + ".HcclCalculator._generate_hccl_op_info"):
            check = HcclCalculator([], CONFIG)
            check.calculate()

    def test_get_hccl_data(self):
        with mock.patch('os.path.exists', return_value=False):
            check = HcclCalculator([], CONFIG)
            self.assertEqual([], check._get_hccl_data())

        with mock.patch('os.path.exists', return_value=True):
            check = HcclCalculator([], CONFIG)
            self.assertEqual([], check._get_hccl_data())

        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_communication_data',
                           return_value=[self.construct_hccl_dto("hccl_op")]):
            check = HcclCalculator([], CONFIG)
            self.assertEqual("hccl_op", check._get_hccl_data()[0].op_name)

    def test_generate_hccl_op_info(self):
        hccl_data = [
            self.construct_hccl_dto("hccl_op1"),
            self.construct_hccl_dto("hccl_op1", timestamp=123457),
            self.construct_hccl_dto("hccl_op2", timestamp=123458)
        ]
        check = HcclCalculator([], CONFIG)
        check._generate_hccl_op_info(hccl_data)
        self.assertEqual(3, len(check._hccl_data))

    def test_ms_run(self):
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch(NAMESPACE + '.HcclCalculator.calculate'), \
             mock.patch(NAMESPACE + '.HcclCalculator.save'), \
             mock.patch(NAMESPACE + '.HcclCalculator.process'):
            key = HcclCalculator([], CONFIG)
            key.ms_run()

    def test_process(self):
        db_manager = DBManager()
        res = db_manager.create_table('hccl.db')
        with mock.patch(NAMESPACE + '.HcclCalculator._judge_precess_again', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            with mock.patch(NAMESPACE + '.HcclCalculator._is_table_need_to_create',
                            return_value=False), \
                    mock.patch(NAMESPACE + '.logging.warning'):
                check = HcclCalculator([], CONFIG)
                result = check.process()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.HcclCalculator._is_table_need_to_create',
                            return_value=True):
                with mock.patch(NAMESPACE + '.HcclCalculator.create_table'), \
                        mock.patch(NAMESPACE + '.HcclCalculator._create_time_data'), \
                        mock.patch(NAMESPACE + '.HcclCalculator._create_report'):
                    check = HcclCalculator([], CONFIG)
                    check.process()
        db_manager.destroy(res)

    def test_is_table_need_to_create(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True):
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
                check = HcclCalculator([], CONFIG)
                result = check._is_table_need_to_create()
            self.assertTrue(result)
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                check = HcclCalculator([], CONFIG)
                result = check._is_table_need_to_create()
            self.assertFalse(result)

    def test_create_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('hccl.db')
        hccl_time_create_sql = "CREATE TABLE IF NOT EXISTS hccl_op_time(op_type TEXT,begin NUMERIC," \
                               "end NUMERIC,duration NUMERIC)"
        hccl_report_create_sql = " CREATE TABLE IF NOT EXISTS hccl_op_report(op_type TEXT," \
                                 "occurrences TEXT,total_time NUMERIC,min NUMERIC,avg NUMERIC,max NUMERIC,ratio TEXT)"
        with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                        return_value=(None, None)), \
                mock.patch(NAMESPACE + '.logging.error'), \
                pytest.raises(ProfException) as error:
            check = HcclCalculator([], CONFIG)
            check.create_table('test')
            self.assertEqual(error.value.code, 10)
        with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                        return_value=res):
            with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                            return_value=hccl_time_create_sql):
                check = HcclCalculator([], CONFIG)
                check.create_table('test')
            with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                            return_value=hccl_report_create_sql):
                check = HcclCalculator([], CONFIG)
                check.create_table('test')
        res[1].execute("drop table hccl_op_time")
        res[1].execute("drop table hccl_op_report")
        db_manager.destroy(res)

    def test_create_time_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.HcclCalculator._get_hccl_time_data', return_value=[[1]]), \
             mock.patch(NAMESPACE + '.DBManager.executemany_sql'):
            check = HcclCalculator([], CONFIG)
            check.conn = True
            check.curs = True
            getattr(check, "_create_time_data")()

    def test_get_hccl_time_data(self):
        with mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                        return_value={}), \
             mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene', return_value="step_info"), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = HcclCalculator([], CONFIG)
            result = check._get_hccl_time_data()
            self.assertEqual(result, [])
        with mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                        return_value={(1, 1)}), \
             mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene', return_value="step_info"), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = HcclCalculator([], CONFIG)
            result = check._get_hccl_time_data()
            self.assertEqual(result, [1])

    def test_get_hccl_op_report_sql(self):
        check = HcclCalculator([], CONFIG)
        result = check._get_hccl_op_report_sql()
        self.assertEqual("select op_type, count(op_type), sum(duration) as total_time, " \
                         "min(duration) as min, sum(duration)/count(op_type) as avg, " \
                         "max(duration) as max from hccl_op_time group by op_type",
                          result)

    def test_cal_total(self):
        task_time = {
                     '1': {'op_type': '1', 'count': 2, 'duration': 4, 'min': 1, 'max': 3, 'avg': 2},
                     '2': {'op_type': '2', 'count': 1, 'duration': 3, 'min': 1, 'max': 1, 'avg': 1}
        }
        check = HcclCalculator([], CONFIG)
        _cal_total = getattr(check, "_cal_total")
        result = _cal_total(task_time)
        self.assertEqual(result, 7)

    def test_create_report(self):
        db_manager = DBManager()
        res = db_manager.create_table('hccl.db')
        curs = res[1]
        curs.execute("CREATE TABLE IF NOT EXISTS hccl_op_time(op_type TEXT,begin NUMERIC," \
                     "end NUMERIC,duration NUMERIC)")
        curs.execute("CREATE TABLE IF NOT EXISTS hccl_op_report(op_type TEXT," \
                     "occurrences TEXT,total_time NUMERIC,min NUMERIC,avg NUMERIC,max NUMERIC,ratio TEXT)")
        res[0].commit()
        sql = "select op_type, count(op_type), sum(duration) as total_time," \
              "min(duration) as min, sum(duration)/count(op_type) as avg," \
              "max(duration) as max from hccl_op_time group by op_type"

        with mock.patch(NAMESPACE + '.HcclCalculator._get_hccl_op_report_sql', return_value=sql):
            check = HcclCalculator([], CONFIG)
            check.conn, check.curs = res[0], res[1]
            check._create_report()
        curs.execute("insert into hccl_op_time values ('allreduce', 1, 2, 1)")
        res[0].commit()
        with mock.patch(NAMESPACE + '.HcclCalculator._get_hccl_op_report_sql', return_value=sql):
            check = HcclCalculator([], CONFIG)
            check.conn, check.curs = res[0], res[1]
            check._create_report()
        curs.execute('drop table hccl_op_time')
        curs.execute('drop table hccl_op_report')
        res[0].commit()
        db_manager.destroy(res)