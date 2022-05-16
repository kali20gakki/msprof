import unittest
from unittest import mock

from analyzer.get_op_table_task_time import GetOpTableTsTime
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'analyzer.get_op_table_task_time'
CONFIG = {'result_dir': '', 'device_id': '0', 'iter_id': 1, 'model_id': -1}


class TestGetOpTableTsTime(unittest.TestCase):
    pass
