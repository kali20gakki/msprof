#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
"""
import collections

import os
import unittest

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.path_manager import PathManager
from constant.constant import clear_dt_project
from mscalculate.cann.additional_record import AdditionalRecord
from mscalculate.cann.cann_analysis_gear import ACLGear
from mscalculate.cann.cann_analysis_gear import ModelGear
from mscalculate.cann.cann_analysis_gear import NodeGear
from mscalculate.cann.cann_analysis_gear import TaskGear
from mscalculate.cann.cann_database import AdditionalRecordDatabase
from mscalculate.cann.cann_database import ApiDataDatabase
from mscalculate.cann.event import Event
from profiling_bean.db_dto.api_data_dto import ApiDataDto
from profiling_bean.db_dto.ctx_id_dto import CtxIdDto
from profiling_bean.db_dto.fusion_op_info_dto import FusionOpInfoDto
from profiling_bean.db_dto.hccl_info_dto import HCCLInfoDto
from profiling_bean.db_dto.mem_copy_info_dto import MemCopyInfoDto
from profiling_bean.db_dto.node_basic_info_dto import NodeBasicInfoDto
from profiling_bean.db_dto.task_track_dto import TaskTrackDto
from profiling_bean.db_dto.tensor_info_dto import TensorInfoDto


class TestCANNAnalysisGear(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_CANNAnalysisGear")
    PROF_HOST_DIR = os.path.join(DIR_PATH, 'PROF1', 'host')
    event_col = collections.namedtuple('Event', 'level, thread_id, start, end, struct_type, item_id')

    @staticmethod
    def create_api_event(event_c):
        api = ApiDataDto()
        api.level = event_c.level
        api.thread_id = event_c.thread_id
        api.start = event_c.start
        api.end = event_c.end
        api.struct_type = event_c.struct_type
        api.item_id = event_c.item_id
        event = ApiDataDatabase().put(api)
        return event

    @staticmethod
    def create_addition_record(dto, time, struct_type=""):
        record = AdditionalRecord(dto, time, struct_type)
        event = AdditionalRecordDatabase().put(record)
        return record

    def setUp(self) -> None:
        clear_dt_project(self.DIR_PATH)
        path = os.path.join(self.PROF_HOST_DIR, 'sqlite')
        os.makedirs(path)

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_acl_gear(self):
        InfoConfReader()._info_json = {'pid': '10'}
        gear = ACLGear(self.PROF_HOST_DIR)

        event = self.create_api_event(self.event_col(Constant.ACL_LEVEL, 1, 0, 100, "", 0))

        gear.run(event, {})
        gear.flush_data()

        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_ACL_MODULE),
                                          DBNameConstant.TABLE_ACL_DATA, 'thread_id', 1))

    def test_model_gear(self):
        gear = ModelGear(self.PROF_HOST_DIR)
        event1 = self.create_api_event(self.event_col(Constant.MODEL_LEVEL, 1, 0, 100, "ModelLoad", 0))
        event2 = self.create_api_event(self.event_col(Constant.MODEL_LEVEL, 1, 101, 200, "InputCopy", 0))
        event3 = self.create_api_event(self.event_col(Constant.MODEL_LEVEL, 1, 201, 300, "ModelExecute", 0))
        event4 = self.create_api_event(self.event_col(Constant.MODEL_LEVEL, 1, 301, 400, "OutputCopy", 0))

        gear.run(event1, {})
        gear.run(event2, {})
        gear.run(event3, {})
        gear.run(event4, {})
        gear.flush_data()

        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_MODEL_INFO),
                                          DBNameConstant.TABLE_GE_MODEL_LOAD, 'end_time', 100))
        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_MODEL_TIME),
                                          DBNameConstant.TABLE_GE_MODEL_TIME, 'infer_start', 201))

    def test_node_gear(self):
        gear = NodeGear(self.PROF_HOST_DIR)
        event1 = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 100, 101, "step_info", 0))
        event2 = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 200, 201, "step_info", 1))
        event3: Event = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 102, 140, "launch", "conv"))
        event3.additional_record = [
            self.create_addition_record(NodeBasicInfoDto(), 140),
            self.create_addition_record(TensorInfoDto(), 140)
        ]
        event4: Event = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 141, 145, "", 1))
        fusion_op = FusionOpInfoDto()
        fusion_op.op_name = "fusion1"
        event4.additional_record = [self.create_addition_record(fusion_op, 143)]
        event5: Event = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 146, 160, "launch", "hccl1"))
        hccl_node_basic_info = NodeBasicInfoDto()
        hccl_node_basic_info.task_type = "HCCL"
        event5.additional_record = [self.create_addition_record(hccl_node_basic_info, 150)]

        gear.run(event1, {})
        gear.run(event2, {})
        gear.run(event3, {})
        gear.run(event4, {})
        gear.run(event5, {})
        gear.flush_data()

        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_HOST_INFO),
                                           DBNameConstant.TABLE_GE_HOST), 2)
        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_HOST_INFO),
                                          DBNameConstant.TABLE_GE_HOST, 'end_time', 140))

        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                           DBNameConstant.TABLE_GE_STEP_INFO), 2)
        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                          DBNameConstant.TABLE_GE_STEP_INFO, 'tag', 0))

        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_HCCL),
                                           DBNameConstant.TABLE_HCCL_OP), 1)
        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_MODEL_INFO),
                                           DBNameConstant.TABLE_GE_FUSION_OP_INFO), 1)

    def test_task_gear(self):
        gear = TaskGear(self.PROF_HOST_DIR)
        event1 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 100, 101, "api1", 0))
        event2 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 200, 250, "api2", 0))

        mem_dto = MemCopyInfoDto()
        mem_dto.struct_type = "1"
        mem_dto.memcpy_direction = 2
        task_dto = TaskTrackDto()
        task_dto.struct_type = "1"
        task_dto.task_id = 10
        event2.additional_record = [
            self.create_addition_record(mem_dto, 210),
            self.create_addition_record(task_dto, 220)
        ]

        event3: Event = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 258, 300, "launch", "hccl1"))
        event4 = self.create_api_event(self.event_col(Constant.HCCL_LEVEL, 1, 260, 290, "hccl api", 0))
        hccl_info_dto = HCCLInfoDto()
        hccl_info_dto.plane_id = 8
        event4.additional_record = [self.create_addition_record(hccl_info_dto, 277)]
        event5 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 264, 280, "api3", 0))
        task_dto2 = TaskTrackDto()
        task_dto2.task_id = 11
        task_dto2.struct_type = "1"
        event5.additional_record = [self.create_addition_record(task_dto2, 266)]

        event6: Event = self.create_api_event(self.event_col(Constant.NODE_LEVEL, 1, 310, 360, "launch", "conv"))
        node_basic_info_dto = NodeBasicInfoDto()
        node_basic_info_dto.op_type = "1"
        node_basic_info_dto.op_name = "1"
        node_basic_info_dto.timestamp = 360
        tensor_info_dto = TensorInfoDto()
        tensor_info_dto.op_name = "1"
        tensor_info_dto.timestamp = 360
        tensor_info_dto.struct_type = '1'
        ctx_id_dto = CtxIdDto()
        ctx_id_dto.ctx_id = 8
        ctx_id_dto.op_name = "1"
        ctx_id_dto.timestamp = 360
        event6.additional_record = [
            self.create_addition_record(node_basic_info_dto, 360),
            self.create_addition_record(tensor_info_dto, 360),
            self.create_addition_record(ctx_id_dto, 360)
        ]
        event7 = self.create_api_event(self.event_col(Constant.TASK_LEVEL, 1, 320, 350, "api4", 0))
        task_dto3 = TaskTrackDto()
        task_dto3.task_id = 12
        task_dto3.struct_type = "1"
        event7.additional_record = [self.create_addition_record(task_dto3, 333)]

        gear.run(event1, {})
        gear.run(event2, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: Event.invalid_event(),
                          Constant.HCCL_LEVEL: Event.invalid_event()})
        gear.run(event5, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: event3,
                          Constant.HCCL_LEVEL: event4})
        gear.run(event7, {Constant.MODEL_LEVEL: Event.invalid_event(), Constant.NODE_LEVEL: event6,
                          Constant.HCCL_LEVEL: Event.invalid_event()})
        gear.flush_data()

        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_RUNTIME),
                                           DBNameConstant.TABLE_API_CALL), 4)

        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                           DBNameConstant.TABLE_GE_TASK), 1)
        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                          DBNameConstant.TABLE_GE_TASK, 'op_type', "1"))

        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                          DBNameConstant.TABLE_GE_TASK, 'context_id', 8))

        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_GE_INFO),
                                           DBNameConstant.TABLE_GE_TENSOR), 1)

        self.assertEqual(
            DBManager.get_table_data_count(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_HCCL),
                                           DBNameConstant.TABLE_HCCL_TASK), 1)

        self.assertTrue(
            DBManager.check_item_in_table(PathManager.get_db_path(self.PROF_HOST_DIR, DBNameConstant.DB_HCCL),
                                          DBNameConstant.TABLE_HCCL_TASK, 'plane_id', 8))


if __name__ == '__main__':
    unittest.main()
