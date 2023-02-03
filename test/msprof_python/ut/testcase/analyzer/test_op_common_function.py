import sqlite3
import unittest
from unittest import mock

from analyzer.op_common_function import OpCommonFunc
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.batch_counter import BatchCounter

NAMESPACE = 'analyzer.op_common_function'
CONFIG = {'result_dir': '', 'device_id': '0', 'iter_id': 1, 'model_id': -1}


class TestOpCommonFunc(unittest.TestCase):

    def test_deal_batch_id(self):
        data = [('0', 4294967295, 2, 2, 'Cast', 'Cast', 1, '"2,5"', 'DT_INT16', 'ND', '"2,5"', '', 'ND', 0, 'AI_CPU'),
                ('1', 4294967295, 7, 2, 'Cast', 'Cast', 1, '"2,3"', 'DT_INT32', 'ND', '"2,3"', '', 'ND', 0, 'AI_CPU'),
                ('2', 4294967295, 12, 2, 'Cast', 'Cast', 1, '"5,3"', 'DT_FLOAT', 'ND', '"5,3"', '', 'ND', 0, 'AI_CPU'),
                ('3', 4294967295, 17, 2, 'Cast', 'Cast', 1, '"4,3"', 'DT_INT16', 'ND', '"4,3"', '', 'ND', 0, 'AI_CPU')]
        res = OpCommonFunc.deal_batch_id(3, 2, data)
        self.assertEqual(len(res), 4)

    def test_calculate_task_time(self):
        data = [(2, 2, 10521325045461.166, 1223854.1655242443, 1, 0, 1, 0),
                (7, 2, 10521333413950.748, 1093229.167163372, 1, 0, 1, 0)]
        ProfilingScene().init(" ")
        ProfilingScene()._scene = Constant.TRAIN
        res = OpCommonFunc.calculate_task_time(data)
        self.assertEqual(len(res), 2)

    def test_get_wait_time(self):
        result = OpCommonFunc._get_wait_time(10,20.0,30)
        self.assertEqual(result, 0)
