import struct
import unittest

from msparser.stars.sio_parser import SioParser
from profiling_bean.stars.sio_bean import SioDecoder

NAMESPACE = 'msparser.stars.sio_parser'
sample_config = {
    "model_id": 1,
    'iter_id': 'dasfsd',
    'result_dir': 'jasdfjfjs',
    "ai_core_profiling_mode": "task-based",
    "aiv_profiling_mode": "sample-based"
}


class TestSioParser(unittest.TestCase):

    def test_decoder(self):
        check = SioParser(sample_config.get('result_dir'), 'test.db', [])
        sio_decoder = check.decoder.decode(
            struct.pack("HHLQHH11L", 25, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
        self.assertEqual(sio_decoder._func_type, '011001')
        check.preprocess_data()
