import struct
import unittest

from common_func.info_conf_reader import InfoConfReader
from msparser.stars import StarsChipTransParser
from profiling_bean.stars.stars_chip_trans_bean import StarsChipTransBean

NAMESPACE = 'msparser.stars.stars_chip_trans_bean'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestStarsChipTransParser(unittest.TestCase):

    def test_decoder(self):
        check = StarsChipTransParser(sample_config.get('result_dir'), 'test.db', [])
        check._decoder = StarsChipTransBean(struct.pack("2HLQ2H3L2Q4L", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
        res = check.decoder
        self.assertEqual(res.sys_cnt, 0)
