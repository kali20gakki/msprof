import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msparser.runtime.rts_parser import RtsTrackParser
from profiling_bean.prof_enum.data_tag import DataTag
from constant.constant import CONFIG

NAMESPACE = 'msparser.runtime.rts_parser'


class TestRtsTrackParser(unittest.TestCase):
    file_list = {DataTag.RUNTIME_TRACK: ['runtime.task_track.1.slice_0']}

    def test_parse(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.RtsTrackParser._read_rts_file'), \
                mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = RtsTrackParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.RtsTrackParser._read_rts_file'), \
                mock.patch('os.path.getsize', return_value=0), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = RtsTrackParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = RtsTrackParser(self.file_list, CONFIG)
            check.parse()

    def test_read_rts_file(self):
        data = struct.pack('=HHIQ32sIHHQ', 23130, 41, 0,1, b'test', 2, 3, 4, 5)
        with mock.patch('builtins.open', mock.mock_open(read_data=data)),\
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data):
            check = RtsTrackParser(self.file_list, CONFIG)
            check._read_rts_file('test', 256)

    def test_save(self):
        with mock.patch('msmodel.runtime.rts_track_model.RtsModel.init'), \
                mock.patch('msmodel.runtime.rts_track_model.RtsModel.check_db'), \
                mock.patch('msmodel.runtime.rts_track_model.RtsModel.create_table'), \
                mock.patch('msmodel.runtime.rts_track_model.RtsModel.insert_data_to_db'), \
                mock.patch('msmodel.runtime.rts_track_model.RtsModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = RtsTrackParser(self.file_list, CONFIG)
            check._rts_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.RtsTrackParser.parse'), \
                mock.patch(NAMESPACE + '.RtsTrackParser.save'):
            RtsTrackParser(self.file_list, CONFIG).ms_run()
        RtsTrackParser({DataTag.RUNTIME_TRACK:[]}, CONFIG).ms_run()
