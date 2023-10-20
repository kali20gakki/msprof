#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from unittest import mock

from msmodel.step_trace.ts_track_model import TsTrackModel
from model.test_dir_cr_base_model import TestDirCRBaseModel
from common_func.db_name_constant import DBNameConstant

NAMESPACE = 'msmodel.step_trace.ts_track_model'


class TestTsTrackModel(TestDirCRBaseModel):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.TsTrackModel.insert_data_to_db'):
            check = TsTrackModel('test', 'step_trace.db', ['StepTrace', 'TsMemcpy'])
            check.flush('StepTrace', [])

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = TsTrackModel('test', 'step_trace.db', ['StepTrace', 'TsMemcpy'])
            check.create_table('StepTrace')

    def test_get_task_flip_data_should_return(self):
        data = [
            [1, 4367981467, 2, 0],
        ]
        with TsTrackModel(self.PROF_DEVICE_DIR,
                          DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_DEVICE_TASK_FLIP]) as model:
            model.create_table(DBNameConstant.TABLE_DEVICE_TASK_FLIP)
            model.flush(DBNameConstant.TABLE_DEVICE_TASK_FLIP, data)
            result = model.get_task_flip_data()
        self.assertEqual(len(result), 1)
