#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
import unittest
from unittest import mock

from common_func.msvp_constant import MsvpConstant
from viewer.hccl_op_report import ReportHcclStatisticData
from sqlite.db_manager import DBOpen

NAMESPACE = 'viewer.hccl_op_report'


class TestReportHcclStatisticData(unittest.TestCase):

    def test_check_param(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = ReportHcclStatisticData.check_param('', '')
        self.assertFalse(res)

    def test_report_op_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = ReportHcclStatisticData.report_hccl_op('', [])
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_report_op_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS hccl_op_report" \
                     "(op_type, occurrences, total_time, " \
                     "min, avg, max, ratio)"
        data = ((1, 4, 10, 1, 2.5, 4, 100),)

        with DBOpen("hccl.db") as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data("hccl_op_report", data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)):
                res = ReportHcclStatisticData.report_hccl_op('', [])
            self.assertEqual(res[2], 1)
