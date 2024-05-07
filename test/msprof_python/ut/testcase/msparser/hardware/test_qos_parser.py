import os
import shutil
import struct
import unittest

from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.hardware.qos_parser import ParsingQosData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.qos_parser'


class TestParsingQosData(unittest.TestCase):
    file_list = {DataTag.QOS: ['qos.data.0.slice_0', 'qos.info.0.slice_0']}

    @classmethod
    def setUpClass(cls):
        InfoConfReader()._info_json = {"devices": '0'}
        if not os.path.exists('./test/data'):
            os.makedirs('./test/data')
            os.makedirs('./test/sqlite')
            cls.make_qos_data()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree("./test")

    @classmethod
    def make_qos_data(cls):
        data = struct.pack("=QQIIQQIIQQIIQQII", 66, 0, 0, 0, 66, 0, 0, 1, 66, 0, 0, 2, 66, 0, 0, 3)
        info = "0=qos_stream_stub\n"
        with FdOpen("./test/data/qos.data.0.slice_0", operate="wb") as f:
            f.write(data)
        with FdOpen("./test/data/qos.info.0.slice_0") as f:
            f.write(info)

    def test_read_binary_data(self):
        check = ParsingQosData(self.file_list, CONFIG)
        result = check.read_binary_data('qos.data.0.slice_0', "0", "0")
        self.assertEqual(result, 0)

    def test_parse(self):
        check = ParsingQosData(self.file_list, CONFIG)
        check.parse()

    def test_save(self):
        check = ParsingQosData(self.file_list, CONFIG)
        check.qos_data = [[66, 0, 0, 0], [66, 0, 0, 1]]
        check.qos_info = [[0, "qos_stream_stub"]]
        check.save()

    def test_ms_run(self):
        ParsingQosData(self.file_list, CONFIG).ms_run()
