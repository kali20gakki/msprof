import unittest

from constant.constant import CONFIG
from msparser.hardware.ctrl_cpu_parser import ParsingCtrlCPUData
from profiling_bean.prof_enum.data_tag import DataTag


class TestParsingAICPUData(unittest.TestCase):
    file_list = {DataTag.CTRLCPU: ['ctrlcpu.data.0.slice_0']}

    def test_init(self):
        check = ParsingCtrlCPUData(self.file_list, CONFIG)
        self.assertEqual(check.type, 'ctrl')
