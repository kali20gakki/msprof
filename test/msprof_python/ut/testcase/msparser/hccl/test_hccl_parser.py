import json
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msparser.hccl.hccl_parser import HCCLParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hccl.hccl_parser'
MODEL_NAMESPACE = 'msmodel.hccl.hccl_model'


class TestHCCLParser(unittest.TestCase):
    sample_config = {'result_dir': '/tmp/result',
                     'tag_id': 'JOBEJGBAHABDEEIJEDFHHFAAAAAAAAAA',
                     'device_id': '127.0.0.1'
                     }
    file_list = {DataTag.HCCL: ['HCCL.hcom_allReduce_1_1_1.1.slice_0']}

    def test_ms_run(self):
        with mock.patch('os.path.realpath', return_value="/hccl"), \
                mock.patch(NAMESPACE + '.HCCLParser.parse'), \
                mock.patch(NAMESPACE + '.HCCLParser.save'):
            HCCLParser(self.file_list, self.sample_config).ms_run()

        with mock.patch('os.path.realpath', return_value="/hccl"), \
                mock.patch(NAMESPACE + '.HCCLParser.parse'), \
                mock.patch(NAMESPACE + '.HCCLParser.save'):
            HCCLParser({}, self.sample_config).ms_run()

        with mock.patch('os.path.realpath', return_value="/hccl"), \
                mock.patch(NAMESPACE + '.HCCLParser.parse'), \
                mock.patch(NAMESPACE + '.HCCLParser.save'):
            HCCLParser(self.file_list, self.sample_config).ms_run()

    def test_parse(self):
        with mock.patch('os.path.realpath', return_value="/hccl"), \
                mock.patch(NAMESPACE + '.HCCLParser._prepare_for_parse'), \
                mock.patch(NAMESPACE + '.HCCLParser._parse_hccl_data'), \
                mock.patch(NAMESPACE + '.HCCLParser._hccl_trace_data'):
            HCCLParser(self.file_list, self.sample_config).parse()

    def test_prepare_for_parse(self):
        with mock.patch('os.path.realpath', return_value="/hccl"), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch(NAMESPACE + '.check_dir_writable'), \
                mock.patch('os.makedirs'):
            HCCLParser(self.file_list, self.sample_config)._prepare_for_parse()

        with mock.patch('os.path.realpath', return_value="/hccl"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.check_dir_writable'), \
                mock.patch('shutil.rmtree'), \
                mock.patch('os.makedirs'):
            HCCLParser(self.file_list, self.sample_config)._prepare_for_parse()

    def test_hccl_trace_data(self):
        hccl_data = {"traceEvents": [{'tid': 2, 'pid': '1', 'ts': 496046552109.73, 'dur': 0.0, 'ph': 'X',
                                      'name': 'Notify Wait',
                                      'args': {'notify id': 4294967344, 'duration estimated': 0.0, 'stage': 4294967295,
                                               'step': 4294967385, 'bandwidth': 'NULL', 'stream id': 1, 'task id': 1,
                                               'task type': 'Notify Wait', 'src rank': 0, 'dst rank': 1,
                                               'transport type': 'LOCAL', 'size': None}}]}
        json_data = json.dumps(hccl_data)
        with mock.patch('os.path.realpath', return_value="/hccl"), \
                mock.patch('os.listdir', return_value=["a"]), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open',
                           mock.mock_open(read_data=json_data)):
            InfoConfReader()._info_json = {"DeviceInfo": [{"id": 5,
                                                           "hwts_frequency": "100",
                                                           "aic_frequency": "1000",
                                                           "aiv_frequency": "1000"}]}
            HCCLParser(self.file_list, self.sample_config)._hccl_trace_data()

    def test_save(self):
        with mock.patch('os.path.realpath', return_value="/hccl"), \
                mock.patch(MODEL_NAMESPACE + '.HCCLModel.init'), \
                mock.patch(MODEL_NAMESPACE + '.HCCLModel.flush'), \
                mock.patch(MODEL_NAMESPACE + '.HCCLModel.finalize'), \
                mock.patch('shutil.rmtree'):
            hccl = HCCLParser(self.file_list, self.sample_config)
            hccl._hccl_data = ["a"]
            hccl.save()