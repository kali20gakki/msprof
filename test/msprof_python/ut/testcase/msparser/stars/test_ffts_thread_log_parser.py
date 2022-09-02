import struct
import unittest

from msparser.stars.ffts_log_parser import FftsLogParser, FftsLogDecoder

NAMESPACE = 'msparser.stars.ffts_log_parser'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestFftsThreadLogDecoder(unittest.TestCase):
    def test_init(self):
        args = (1, 2, 3, 74, 8, 9, 7, 748, 8, 7, 1)
        FftsLogDecoder(args)


class TestFftsThreadLogParser(unittest.TestCase):
    def test_init(self):
        result_dir = ''
        db = ''
        table_list = ''
        FftsLogParser(result_dir, db, table_list)

    def test_decoder(self):
        check = FftsLogParser(sample_config.get('result_dir'), 'test.db', [])
        check._decoder = FftsLogDecoder(struct.pack("4HQ2H3L4Q", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
        check.decoder


if __name__ == '__main__':
    unittest.main()
