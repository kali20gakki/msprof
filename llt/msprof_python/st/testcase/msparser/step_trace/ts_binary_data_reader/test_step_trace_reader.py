import struct
import unittest
from msparser.step_trace.ts_binary_data_reader.step_trace_reader import StepTraceReader


class TestStepTraceReader(unittest.TestCase):
    def test_read_binary_data(self):
        # index id, model id, timestamp, stream_id, task_id, tag_id
        expect_res = [(1, 1, 101612167908, 3, 1, 0)]

        file_data = struct.pack("=BBH4BQQQHHHH", 1, 10, 32, 0, 0, 0, 0, 101612167908, 1, 1, 3, 1, 0, 0)

        step_trace_reader = StepTraceReader()
        step_trace_reader.read_binary_data(file_data)
        self.assertEqual(expect_res, step_trace_reader.data)
