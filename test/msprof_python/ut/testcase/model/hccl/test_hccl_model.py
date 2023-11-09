#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.

import os
from unittest import mock

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ProfilingScene
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.hccl.hccl_model import HCCLModel
from msmodel.hccl.hccl_model import HcclViewModel
from mscalculate.hccl.hccl_task import HcclTask
from profiling_bean.prof_enum.data_tag import DataTag
from sqlite.db_manager import DBOpen

NAMESPACE = 'msmodel.hccl.hccl_model'


class TestHCCLModel(TestDirCRBaseModel):
    sample_config = {'result_dir': '/tmp/result',
                     'tag_id': 'JOBEJGBAHABDEEIJEDFHHFAAAAAAAAAA',
                     'device_id': '127.0.0.1'
    }
    file_list = {DataTag.HCCL: ['HCCL.hcom_allReduce_1_1_1.1.slice_0']}

    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_HCCL_MODEL")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')
    PROF_DEVICE_DIR = os.path.join(PROF_DIR, 'device')
    PROF_HOST_DIR = os.path.join(PROF_DIR, 'host')

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HCCLModel.insert_data_to_db'):
            HCCLModel("", [" "]).flush([])

    def test_get_hccl_data(self):
        data = [1, 2, 3, 4,
                "{'notify id': 4294967840, 'duration estimated': 0.8800048828125, 'stage': 4294967295, "
                "'step': 4294967385, 'bandwidth': 'NULL', 'stream id': 8, 'task id': 34, 'task type': 'Notify Record',"
                " 'src rank': 2, 'dst rank': 1, 'transport type': 'SDMA', 'size': None, 'tag': 'all2allvc_1_5'}"]
        col = ["hccl_name", "plane_id", "timestamp", "duration", "args"]
        create_sql = "create table IF NOT EXISTS {0} " \
                     "(name TEXT, " \
                     "plane_id INTEGER, " \
                     "timestamp REAL, " \
                     "duration REAL, " \
                     "args TEXT)".format(DBNameConstant.TABLE_HCCL_SINGLE_DEVICE)
        test = HcclTask()
        for index, i in enumerate(data):
            if hasattr(test, col[index]):
                setattr(test, col[index], i)
        with DBOpen(DBNameConstant.DB_HCCL) as db_open:
            db_open.create_table(create_sql)
            with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[test]):
                check = HCCLModel("", [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
                check.cur = db_open.db_curs
                check.get_hccl_data()

    def test_get_hccl_task_data_should_return_empty_when_attach_db_failed(self):
        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=False):
            model = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
            ret = model.get_hccl_task_data()
            self.assertEqual([], ret)

    def test_get_hccl_task_data_should_return_empty_when_device_id_na(self):
        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            InfoConfReader()._info_json = {}
            model = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
            ret = model.get_hccl_task_data()
            self.assertEqual([], ret)

    def test_get_hccl_task_data_should_return_diff_result_when_query_diff_deviceid(self):
        # model_id, index_id, name, group_name, plane_id, timestamp, duration,
        # stream_id, task_id, context_id, batch_id, device_id, args
        hccl_task_data = [
            (1, -1, "Memcpy", "1", 1, 1, 1, 100, 200, 300, 0, 0, 0, "1", 1, 1, "1", 20, "a", "b", "2"),
            (1, -1, "Notify_Wait", "1", 1, 1, 1, 102, 202, 302, 0, 0, 0, "1", 1, 1, "1", 20, "a", "b", "2"),
            (1, -1, "Notify_Record", "1", 1, 1, 1, 103, 203, 303, 0, 0, 0, "1", 1, 1, "1", 20, "a", "b", "2"),
            (1, -1, "Memcpy", "1", 1, 1, 1, 999, 204, 304, 0, 0, 0, "1", 1, 1, "1", 20, "a", "b", "2"),
            (1, -1, "Memcpy", "1", 1, 1, 1, 105, 205, 305, 0, 1, 0, "1", 1, 1, "1", 20, "a", "b", "2"),
            (1, -1, "Memcpy", "1", 1, 1, 1, 999, 206, 306, 0, 2, 0, "1", 1, 1, "1", 20, "a", "b", "2"),
        ]

        # model_id, index_id, stream_id, task_id, context_id, batch_id, start_time,
        # duration, host_task_type, device_task_type, connection_id
        ascend_task_data = [
            (1, -1, 100, 200, 300, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 102, 202, 302, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 103, 203, 303, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 104, 204, 304, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 105, 205, 305, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
            (1, -1, 106, 206, 306, 0, 1, 1, "PROFILING_ENABLE", "PLACE_HOLDER_SQE", 52212),
        ]

        model = HcclViewModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_HCCL,
                              [DBNameConstant.TABLE_HCCL_TASK, DBNameConstant.TABLE_ASCEND_TASK])

        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_TASK, hccl_task_data)
        model.insert_data_to_db(DBNameConstant.TABLE_ASCEND_TASK, ascend_task_data)

        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            try:
            # test on device_0 matched case
                InfoConfReader()._info_json = {"devices": "0"}
                ret = model.get_hccl_task_data()
                self.assertEqual(len(ret), 3)

                # test on device_1 matched case
                InfoConfReader()._info_json = {"devices": "1"}
                ret = model.get_hccl_task_data()
                self.assertEqual(len(ret), 1)

                # test on device_2 matched case
                InfoConfReader()._info_json = {"devices": "2"}
                ret = model.get_hccl_task_data()
                self.assertEqual(len(ret), 0)
                InfoConfReader()._info_json = {}
            finally:
                model.finalize()


    def test_get_hccl_ops_should_return_empty_when_device_id_na(self):
        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            InfoConfReader()._info_json = {}
            model = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
            ret = model.get_hccl_ops(1, 1)
            self.assertEqual([], ret)

    def test_get_hccl_ops_should_return_diff_result_when_query_diff_device_id_in_op_scene(self):
        # device_id, model_id, index_id, thread_id, op_name, task_type, op_type, begin, end, is_dynamic, connection_id
        hccl_op_data = [
            (0, 1, 1, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 1, 1, 1, 1),
            (0, 1, 1, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 2, 1, 1, 1),
            (0, 1, 1, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 3, 1, 1, 1),
            (1, 1, 1, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 3, 1, 1, 1),
            (2, 1, 1, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 4, 1, 1, 1),
        ]

        model = HcclViewModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_OP])
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP, hccl_op_data)

        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            scene = ProfilingScene()
            scene._scene = Constant.SINGLE_OP

            # test on device_0 matched case
            InfoConfReader()._info_json = {"devices": "0"}
            ret = model.get_hccl_ops(1, 1)
            self.assertEqual(len(ret), 3)

            # test on device_1 matched case
            InfoConfReader()._info_json = {"devices": "1"}
            ret = model.get_hccl_ops(1, 1)
            self.assertEqual(len(ret), 1)

            InfoConfReader()._info_json = {}
            scene._scene = None

        model.finalize()

    def test_get_hccl_ops_should_return_diff_result_when_query_diff_device_id_in_graph_scene(self):
        # device_id, model_id, index_id, thread_id, op_name, task_type, op_type, begin, end, is_dynamic, connection_id
        hccl_op_data = [
            (0, 1, 1, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 1, 1, 1, 1),
            (0, 2, 2, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 2, 1, 1, 1),
            (0, 2, 2, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 3, 1, 1, 1),
            (1, 1, 1, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 3, 1, 1, 1),
            (2, 3, 3, 1, "hcom_broadcast_", "HCCL", "hcom_broadcast_", 4, 1, 1, 1),
        ]

        model = HcclViewModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_OP])
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP, hccl_op_data)

        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            scene = ProfilingScene()
            scene._scene = Constant.STEP_INFO
            ProfilingScene().set_all_export(False)

            # test on device_0 matched case
            InfoConfReader()._info_json = {"devices": "0"}
            ret = model.get_hccl_ops(2, 2)
            self.assertEqual(len(ret), 2)

            # test on device_1 matched case
            InfoConfReader()._info_json = {"devices": "1"}
            ret = model.get_hccl_ops(1, 1)
            self.assertEqual(len(ret), 1)

            InfoConfReader()._info_json = {}
            scene._scene = None

        model.finalize()

    def test_get_hccl_op_data_sql(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
            check.get_hccl_op_data()

    def test_get_task_time_sql(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
            check.get_task_time_sql()

    def test_get_hccl_op_data_by_group_sql(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
            check.get_hccl_op_data_by_group()

    def test_get_hccl_op_data_by_group_sql_should_return_empty_when_no_master(self):
        # model_id, index_id, op_name, iteration, hccl_name,
        # group_name, first_timestamp, plane_id, timestamp, duration,
        # is_dynamic, task_type, op_type, connection_id, is_master
        hccl_op_data = [
            (4294967295, -1, "hcom_allReduce__111_5939", 0, "Notify_Wait",
             9402293354310575111, 642053765422, 1, 6286049445495.44, 1324686.46679688,
             1, "HCCL", "hcom_allReduce_", 733589, 0),
            (4294967295, -1, "hcom_allReduce__111_5939", 0, "Notify_Wait",
             9402293354310575111, 642070508282, 2, 6286050773361.97, 1409648.16503906,
             1, "HCCL", "hcom_allReduce_", 733589, 0),
        ]
        model = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_HCCL_SINGLE_DEVICE, hccl_op_data)
        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True):
            hccl_result = model.get_hccl_op_data_by_group()
            self.assertEqual(len(hccl_result), 0)
        model.finalize()

    def test_get_hccl_op_time_section_sql(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
            check.get_hccl_op_time_section()

    def test_create_table_by_name_should_drop_table_when_tabel_exist(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
            check.create_table_by_name(table_name='test_name')

    def test_create_table_by_name_should_not_drop_table_when_tabel_not_exist(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL_SINGLE_DEVICE, [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE])
            check.create_table_by_name(table_name='test_name')