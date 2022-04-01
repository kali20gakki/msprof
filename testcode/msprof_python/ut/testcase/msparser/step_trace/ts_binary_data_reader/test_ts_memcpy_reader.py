import struct
import unittest
from msparser.step_trace.ts_binary_data_reader.ts_memcpy_reader import TsMemcpyReader


class TestTsMemcpyReader(unittest.TestCase):
    def test_read_binary_data(self):
        # timestamp, stream_id, task_id, tag_state
        expect_res = [(101612167908, 1, 1, 0)]

        file_data = struct.pack("=BBH4BQHHBBHQQ", 0, 11, 32, 0, 0, 0, 0, 101612167908, 1, 1, 0, 0, 0, 0, 0)

        ts_memcpy_reader = TsMemcpyReader()
        ts_memcpy_reader.read_binary_data(file_data)
        self.assertEqual(expect_res, ts_memcpy_reader.data)
