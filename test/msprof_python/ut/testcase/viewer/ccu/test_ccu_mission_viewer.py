#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

import os
import unittest
import shutil
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.ccu.ccu_mission_dto import OriginMissionDto
from profiling_bean.db_dto.ccu.ccu_channel_dto import OriginChannelDto
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginTaskInfoDto
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginGroupInfoDto
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginWaitSignalInfoDto
from profiling_bean.db_dto.step_trace_dto import IterationRange
from viewer.ccu.ccu_mission_viewer import CCUMissionViewer
from profiling_bean.prof_enum.data_tag import DataTag
from msparser.add_info.ccu_add_info_parser import CCUAddInfoParser
from msparser.hardware.ccu_mission_parser import CCUMissionParser
from msparser.hardware.ccu_channel_parser import CCUChannelParser

NAMESPACE = 'msmodel.hardware.ccu_mission_model'


class TestCCUMissionViewer(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), "ccu_add_info")
    HOST_PATH = os.path.join(DIR_PATH, "host")
    DEVICE_PATH = os.path.join(DIR_PATH, "device")
    HOST_SQLITE_PATH = os.path.join(HOST_PATH, "sqlite")
    DEVICE_SQLITE_PATH = os.path.join(DEVICE_PATH, "sqlite")
    file_list = {}
    HOST_CONFIG = {
        'result_dir': HOST_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }
    DEVICE_CONFIG = {
        'result_dir': DEVICE_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    def setUp(self):
        if not os.path.exists(self.DIR_PATH):
            os.mkdir(self.DIR_PATH)
        if not os.path.exists(self.HOST_PATH):
            os.mkdir(self.HOST_PATH)
        if not os.path.exists(self.DEVICE_PATH):
            os.mkdir(self.DEVICE_PATH)
        if not os.path.exists(self.HOST_SQLITE_PATH):
            os.mkdir(self.HOST_SQLITE_PATH)
        if not os.path.exists(self.DEVICE_SQLITE_PATH):
            os.mkdir(self.DEVICE_SQLITE_PATH)
        if os.path.exists(self.HOST_SQLITE_PATH):
            add_info_parser = CCUAddInfoParser(self.file_list, self.HOST_CONFIG)
            add_info_parser._ccu_add_info_data = {
                DataTag.CCU_TASK: [[0, 1, '9581505218827963271', '7415574198778220483', 0, 2, 1, 0, 1, 1, 0]],
                DataTag.CCU_WAIT_SIGNAL:
                    [[0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 39, 1, 210, 2, 2, 1],
                     [0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 40, 1, 211, 2, 2, 1]],
                DataTag.CCU_GROUP:
                    [[0, '1362511707695072317', '7415574198778220483', 0, 2, 1, 1, 0, 1, 119, 255,
                      'SUM', 'INT32', 'INT32', 8192, 2, 1],
                     [0, '9129927687939955350', '7415574198778220483', 0, 2, 1, 1, 0, 1, 197, 255,
                      'RESERVED', 'RESERVED', 'RESERVED', 8192, 2, 1]]
            }
            add_info_parser.save()
        if os.path.exists(self.DEVICE_SQLITE_PATH):
            check = CCUMissionParser(self.file_list, self.DEVICE_CONFIG)
            check.mission_data = [
                [1, 0, 0, 0, 0, 74, 133158368126, 15, 133158368126],
                [1, 0, 0, 0, 0, 74, 133158368126, 14, 133158368126]
            ]
            check.save()

            check = CCUChannelParser(self.file_list, self.DEVICE_CONFIG)
            check.channel_data = [
                [0, 4919982785935, 25011, 25011, 25011],
                [1, 4919982785935, 4181298, 1435, 1068902],
                [2, 4919982785936, 13681, 13681, 13681]
            ]
            check.save()

    def teardown_class(self):
        if os.path.exists(self.DIR_PATH):
            shutil.rmtree(self.DIR_PATH)

    def test_ccu_mission_viewer_should_return_true_when_run_get_timeline_data_success(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        params = {'data_type': 'ccu_mission',
                  'project': 'prof_114514',
                  'device_id': 0,
                  'export_type': 'timeline',
                  'iter_id': 1,
                  'export_format': None,
                  'model_id': 4294967295,
                  'export_dump_folder': 'timeline'}
        configs = {'handler': '_get_ccu_mission_data',
                   'headers': ['Stream ID', 'Task Id', 'Instruction ID', 'Instruction Start Time(us)',
                               'Instruction Duration(us)', 'Notify Instruction ID', 'Notify Rank ID',
                               'Notify Duration(us)'], 'db': 'ccu.db', 'table': 'OriginMission', 'unused_cols': []}
        check = CCUMissionViewer(configs, params)
        check.get_timeline_data()

    def test_format_mission_summary_data_should_return_formatted_data(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        datas = [
            OriginMissionDto(stream_id=0, task_id=0, lp_instr_id=0, lp_start_time=0, lp_end_time=0,
                             setckebit_instr_id=74, setckebit_start_time=0, rel_id=15, rel_end_time=0),
            OriginMissionDto(stream_id=0, task_id=0, lp_instr_id=0, lp_start_time=0, lp_end_time=0,
                             setckebit_instr_id=74, setckebit_start_time=0, rel_id=14, rel_end_time=0)
        ]
        params = {'data_type': 'ccu_mission',
                  'project': 'prof_114514',
                  'device_id': 0,
                  'export_type': 'timeline',
                  'iter_id': 1,
                  'export_format': None,
                  'model_id': 4294967295,
                  'export_dump_folder': 'timeline'}
        with mock.patch(NAMESPACE + '.CCUViewerMissionModel.check_table', return_value=True):
            check = CCUMissionViewer({}, params)
            ret = check.format_mission_summary_data(datas)
            self.assertEqual(2, len(ret))
            self.assertEqual(74, ret[0][5])

    def test_get_trace_timeline_should_return_timeline_format(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        ccu_data_dict = {
            DataTag.CCU_MISSION: [
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=0, setckebit_instr_id=34, rel_id=15,
                                 start_time=1324138574522, end_time=1324138574522, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=0, setckebit_instr_id=34, rel_id=14,
                                 start_time=1324138574522, end_time=1324138574522, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=0, setckebit_instr_id=40, rel_id=1,
                                 start_time=1324138574641, end_time=1324138574641, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=0, setckebit_instr_id=40, rel_id=0,
                                 start_time=1324138574641, end_time=1324138574641, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=119, setckebit_instr_id=0, rel_id=0,
                                 start_time=1324138575375, end_time=1324138595272, time_type='LoopGroup',
                                 lp_start_time=None, lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=197, setckebit_instr_id=0, rel_id=0,
                                 start_time=1324138595993, end_time=1324138600670, time_type='LoopGroup',
                                 lp_start_time=None, lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=197, setckebit_instr_id=205, rel_id=2,
                                 start_time=1324138600757, end_time=1324138600757, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=197, setckebit_instr_id=205, rel_id=1,
                                 start_time=1324138600757, end_time=1324138601884, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=197, setckebit_instr_id=205, rel_id=0,
                                 start_time=1324138600757, end_time=1324138600757, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None)],
            DataTag.CCU_CHANNEL: [
                OriginChannelDto(channel_id=0, timestamp=1324138605307, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=1, timestamp=1324138605307, max_bw=14299535, min_bw=14299535,
                                 avg_bw=14299535),
                OriginChannelDto(channel_id=2, timestamp=1324138605308, max_bw=119258, min_bw=202,
                                 avg_bw=25831),
                OriginChannelDto(channel_id=3, timestamp=1324138605308, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=4, timestamp=1324138605309, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=5, timestamp=1324138605310, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=6, timestamp=1324138605310, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=124, timestamp=1324138605548, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=125, timestamp=1324138605549, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=126, timestamp=1324138605550, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=127, timestamp=1324138605550, max_bw=0, min_bw=0, avg_bw=0)],
            DataTag.CCU_TASK: [OriginTaskInfoDto(version=0, work_flow_mode=1, item_id='9581505218827963271',
                                                 group_name='7415574198778220483', rank_id=0, rank_size=2,
                                                 stream_id=1, task_id=0, die_id=1, mission_id=1, instr_id=0)],
            DataTag.CCU_WAIT_SIGNAL: [OriginWaitSignalInfoDto(version=0, item_id='6823696491211891432',
                                                              group_name='7415574198778220483', rank_id=0,
                                                              work_flow_mode=1, rank_size=2, stream_id=1,
                                                              task_id=0, die_id=1, instr_id=38, mission_id=1,
                                                              cke_id=209, mask=2, channel_id=2,
                                                              remote_rank_id=1),
                                      OriginWaitSignalInfoDto(version=0, item_id='6823696491211891432',
                                                              group_name='7415574198778220483', rank_id=0,
                                                              work_flow_mode=1, rank_size=2, stream_id=1,
                                                              task_id=0, die_id=1, instr_id=39, mission_id=1,
                                                              cke_id=210, mask=2, channel_id=2,
                                                              remote_rank_id=1),
                                      OriginWaitSignalInfoDto(version=0, item_id='6823696491211891432',
                                                              group_name='7415574198778220483', rank_id=0,
                                                              work_flow_mode=1, rank_size=2, stream_id=1,
                                                              task_id=0, die_id=1, instr_id=40, mission_id=1,
                                                              cke_id=211, mask=2, channel_id=2,
                                                              remote_rank_id=1),
                                      OriginWaitSignalInfoDto(version=0, item_id='6823696491211891432',
                                                              group_name='7415574198778220483', rank_id=0,
                                                              work_flow_mode=1, rank_size=2, stream_id=1,
                                                              task_id=0, die_id=1, instr_id=205, mission_id=1,
                                                              cke_id=208, mask=2, channel_id=2,
                                                              remote_rank_id=1)],
            DataTag.CCU_GROUP: [OriginGroupInfoDto(version=0, item_id='1362511707695072317',
                                                   group_name='7415574198778220483', rank_id=0, work_flow_mode=1,
                                                   rank_size=2, stream_id=1, task_id=0, die_id=1, instr_id=119,
                                                   mission_id=255, reduce_op_type='SUM', input_data_type='INT32',
                                                   output_data_type='INT32', data_size=8192, channel_id=2,
                                                   remote_rank_id=1),
                                OriginGroupInfoDto(version=0, item_id='9129927687939955350',
                                                   group_name='7415574198778220483', rank_id=0, work_flow_mode=1,
                                                   rank_size=2, stream_id=1, task_id=0, die_id=1, instr_id=197,
                                                   mission_id=255, reduce_op_type='RESERVED',
                                                   input_data_type='RESERVED', output_data_type='RESERVED',
                                                   data_size=8192, channel_id=2, remote_rank_id=1)]}
        params = {'data_type': 'ccu_mission',
                  'project': 'prof_114514',
                  'device_id': 0,
                  'export_type': 'timeline',
                  'iter_id': 1,
                  'export_format': None,
                  'model_id': 4294967295,
                  'export_dump_folder': 'timeline'}
        with mock.patch(NAMESPACE + '.CCUViewerMissionModel.check_table', return_value=True):
            check = CCUMissionViewer({}, params)
            ret = check.get_trace_timeline(ccu_data_dict)
            self.assertEqual(7, len(ret))
