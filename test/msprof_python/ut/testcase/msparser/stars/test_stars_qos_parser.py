import struct
import unittest

from constant.constant import CONFIG
from msparser.data_struct_size_constant import StructFmt
from msparser.stars.stars_qos_parser import StarsQosParser

NAMESPACE = 'msparser.stars.stars_qos_parser'


class TestStarsQosParser(unittest.TestCase):

    def test_stars_qos_parser_should_parse_preprocess_data(self):
        check = StarsQosParser(CONFIG.get('result_dir'), 'test.db', [])
        decoder = check._decoder.decode(
            struct.pack(StructFmt.BYTE_ORDER_CHAR + StructFmt.STARS_QOS_FMT, 24, 27603, 0, 1000, 0, 20, 20, 20, 20, 20,
                        20, 20, 20, 20, 20))
        self.assertEqual(decoder.die_id, 0)
        self.assertEqual(len(decoder.qos_bw_data), 10)
        check.preprocess_data()
