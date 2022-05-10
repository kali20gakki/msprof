import struct
import unittest

from msparser.step_trace.helper.model_with_q_parser import ModelWithQParser


class TestModelWithQParser(unittest.TestCase):
    def test_read_binary_data(self):
        # index id, model id, timestamp, tag_id, event_id

        file_data = struct.pack("=HHIQQIHHQ24B", 1, 1, 1, 101612167908, 1, 1, 3, 1, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

        data_parser = ModelWithQParser()
        data_parser.read_binary_data(file_data)
        self.assertEqual(data_parser._data[0][2], 101612167908)
