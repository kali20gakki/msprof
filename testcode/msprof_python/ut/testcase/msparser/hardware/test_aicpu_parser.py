import unittest

from constant.constant import CONFIG
from msparser.hardware.ai_cpu_parser import ParsingAICPUData
from profiling_bean.prof_enum.data_tag import DataTag


class TestParsingAICPUData(unittest.TestCase):
    file_list = {DataTag.AICPU: ['aicpu.data.0.slice_0']}

    def test_init(self):
        check = ParsingAICPUData(self.file_list, CONFIG)
        self.assertEqual(check.type, 'ai')
