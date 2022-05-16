#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from common_func.constant import Constant
from mscalculate.hwts.hwts_calculator import HwtsCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.hwts_log import HwtsLogBean

NAMESPACE = 'mscalculate.hwts.hwts_calculator'


class TestHwtsCalculator(unittest.TestCase):
    file_list = {DataTag.HWTS: ['hwts.data.0.slice_0']}

    def test_ms_run(self):
        ProfilingScene()._scene = "single_op"
        with mock.patch(NAMESPACE + '.HwtsCalculator.calculate'), \
             mock.patch(NAMESPACE + '.HwtsCalculator.save'):
            check = HwtsCalculator(self.file_list, CONFIG)
            check.ms_run()

    def test_prep_data(self):
        start_3_2 = b'\x00\x00\xd3k\x00\x00\x02\x00\xc9o\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        end_3_2 = b'\x11\x00\xd3k\x00\x00\x02\x00\xc4\x80\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        end_3_3 = b'1\x00\xd3k\x00\x00\x03\x00\xcf\x9d\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        start_3_4 = b'@\x00\xd3k\x00\x00\x04\x00\x0e\xa4\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        start_3_5 = b'`\x00\xd3k\x00\x00\x05\x00\xe4\xab\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        end_3_5 = b'q\x00\xd3k\x00\x00\x05\x00\xeb\xac\x91\x06\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': '1150', "hwts_frequency": "38.4"}]}

        check = HwtsCalculator(self.file_list, CONFIG)
        check._log_data.append(HwtsLogBean.decode(start_3_2))
        check._log_data.append(HwtsLogBean.decode(end_3_2))
        check._log_data.append(HwtsLogBean.decode(end_3_3))
        check._log_data.append(HwtsLogBean.decode(start_3_4))
        check._log_data.append(HwtsLogBean.decode(start_3_5))
        check._log_data.append(HwtsLogBean.decode(start_3_5))
        check._log_data.append(HwtsLogBean.decode(end_3_5))
        check._prep_data()

    def test_add_batch_id(self):
        expect_single_res = [[1, 2, 0], [3, 4, 0]]
        prep_data_res = [[1, 2], [3, 4]]

        ProfilingScene().init("")
        check = HwtsCalculator(self.file_list, CONFIG)

        ProfilingScene()._scene = Constant.SINGLE_OP
        res = check._add_batch_id(prep_data_res)
        self.assertEqual(res, expect_single_res)

        prep_data_res = [[1, 2], [3, 4]]
        hwts_iter_model = mock.Mock()
        hwts_iter_model.init = mock.Mock(return_value=None)
        hwts_iter_model.get_batch_list = mock.Mock(return_value=[[0], [0]])
        check._iter_model = hwts_iter_model
        ProfilingScene()._scene = Constant.STEP_INFO
        with mock.patch("common_func.msprof_iteration.MsprofIteration.get_iteration_id_by_index_id", return_value=1):
            res = check._add_batch_id(prep_data_res)
        self.assertEqual(res, expect_single_res)
