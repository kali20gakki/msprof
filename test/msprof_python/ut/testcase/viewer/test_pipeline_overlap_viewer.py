#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
import json
import os
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_object import CustomizedNamedtupleFactory
from constant.constant import clear_dt_project
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from profiling_bean.db_dto.time_section_dto import CommunicationTimeSection
from profiling_bean.db_dto.time_section_dto import TimeSectionDto
from viewer.pipeline_overlap_viewer import OverlapType
from viewer.pipeline_overlap_viewer import PipelineOverlapViewer

NAMESPACE = 'viewer.pipeline_overlap_viewer'


class TestPipelineOverlapViewer(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_PipelineOverlapViewer')

    @staticmethod
    def construct_time_section(start, end, op_name=None, class_bean=TimeSectionDto):
        class_bean_tuple = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(class_bean, [])
        return class_bean_tuple(start_time=start, end_time=end, op_name=op_name)

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_get_timeline_data_should_return_empty_data_when_db_not_exist(self):
        InfoJsonReaderManager(info_json=InfoJson(pid=1000)).process()
        with mock.patch('os.path.exists', return_value=False):
            check = PipelineOverlapViewer({}, {
                StrConstant.SAMPLE_CONFIG_PROJECT_PATH: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
            ret = check.get_timeline_data()
            self.assertEqual([], ret)

    def test_get_timeline_data_should_return_only_task_data_when_hccl_db_not_exist(self):
        InfoJsonReaderManager(info_json=InfoJson(pid=1000)).process()
        InfoConfReader()._local_time_offset = 10.0
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.check_table', return_value=False), \
                mock.patch(NAMESPACE + '.OpSummaryModel.get_operator_data_by_task_type',
                           return_value=[self.construct_time_section(1000, 1100, 'aclnnCat_ConcatD_ConcatD'),
                                         self.construct_time_section(1080, 1200, 'aclnnCos_CosAiCore_Cos'),
                                         self.construct_time_section(1280, 1300, 'RotaryMul1')]):
            check = PipelineOverlapViewer({}, {
                StrConstant.PARAM_RESULT_DIR: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
            ret = check.get_timeline_data()
            self.assertEqual(729, len(json.dumps(ret)))

    def test_get_timeline_data_should_return_only_hccl_data_when_op_summay_db_not_exist(self):
        InfoJsonReaderManager(info_json=InfoJson(pid=1000)).process()
        InfoConfReader()._local_time_offset = 10.0
        with mock.patch('os.path.exists', side_effect=[False, True]), \
                mock.patch(NAMESPACE + '.HcclViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_op_time_section',
                           return_value=[self.construct_time_section(1180, 1250, class_bean=CommunicationTimeSection)]):
            check = PipelineOverlapViewer({}, {
                StrConstant.PARAM_RESULT_DIR: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
            ret = check.get_timeline_data()
            self.assertEqual(673, len(json.dumps(ret)))

    def test_get_timeline_data_with_data(self):
        InfoJsonReaderManager(info_json=InfoJson(pid=1000)).process()
        InfoConfReader()._local_time_offset = 10.0
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.OpSummaryModel.get_operator_data_by_task_type',
                           return_value=[self.construct_time_section(1000, 1100, 'aclnnCat_ConcatD_ConcatD'),
                                         self.construct_time_section(1080, 1200, 'aclnnCos_CosAiCore_Cos'),
                                         self.construct_time_section(1280, 1300, 'RotaryMul1')]), \
                mock.patch(NAMESPACE + '.HcclViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_op_time_section',
                           return_value=[self.construct_time_section(1180, 1250, class_bean=CommunicationTimeSection)]):
            check = PipelineOverlapViewer({}, {
                StrConstant.PARAM_RESULT_DIR: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
            ret = check.get_timeline_data()
            self.assertEqual(925, len(json.dumps(ret)))

    def test_format_timeline_data(self):
        time_section = TimeSectionDto()
        time_section.start_time, time_section.end_time = 123456, 123457
        InfoJsonReaderManager(info_json=InfoJson(pid=1000)).process()
        check = PipelineOverlapViewer({}, {
            StrConstant.SAMPLE_CONFIG_PROJECT_PATH: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
        self.assertEqual(1000, check._format_timeline_data(OverlapType.FREE_TIME, time_section)[1])
