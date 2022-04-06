import struct
import unittest

from msparser.stars.acc_pmu_parser import AccPmuParser
from profiling_bean.stars.acc_pmu import AccPmuDecoder

NAMESPACE = 'msparser.stars.acc_pmu_parser'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestAccPmuParser(unittest.TestCase):

    def test_decoder(self):
        check = AccPmuParser(sample_config.get('result_dir'), 'test.db', [])
        check._decoder = AccPmuDecoder(struct.pack("4HQ2H3L4Q", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
