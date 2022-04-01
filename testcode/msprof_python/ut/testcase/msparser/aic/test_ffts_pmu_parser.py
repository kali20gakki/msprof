import sqlite3
import struct
import unittest
from unittest import mock

from msparser.aic.ffts_pmu_parser import FftsPmuParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.aic.ffts_pmu_parser'


class TestFftsPmuParser(unittest.TestCase):
    file_list = {DataTag.AI_CPU: ['ffts_profile.data.0.slice_0']}

    def test_parse(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value=1), \
                mock.patch(NAMESPACE + '.FftsPmuParser._parse_binary_file'):
            check = FftsPmuParser({DataTag.FFTS_PMU: ['stars_soc.data.slice_0']}, {1: 'test'})
            check.parse()

    def test_parse_binary_file(self):
        data = struct.pack("=HHHHLLHHBBBBLL12Q",
                           40, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 8, 9, 4, 5, 8, 9, 7, 5, 7, 8)
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.check_file_readable'):
            check = FftsPmuParser(self.file_list, {1: 'test'})
            check._parse_binary_file('stars_soc.data.slice_0')

    def test_save(self):
        with mock.patch(NAMESPACE + '.FftsPmuModel.init'), \
                mock.patch(NAMESPACE + '.FftsPmuModel.flush'), \
                mock.patch(NAMESPACE + '.FftsPmuModel.finalize'):
            check = FftsPmuParser(self.file_list, {1: 'test'})
            check._data_list = [(1,)]
            check.save()
        with mock.patch(NAMESPACE + '.logging.warning'):
            check = FftsPmuParser(self.file_list, {1: 'test'})
            check.save()
        with mock.patch(NAMESPACE + '.FftsPmuModel.init', side_effect=sqlite3.DatabaseError), \
                mock.patch(NAMESPACE + '.FftsPmuModel.flush'), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.FftsPmuModel.finalize'):
            check = FftsPmuParser(self.file_list, {1: 'test'})
            check._data_list = [(1,)]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.FftsPmuParser.parse'), \
                mock.patch(NAMESPACE + '.FftsPmuParser.save'):
            check = FftsPmuParser({DataTag.FFTS_PMU: ['stars_soc.data.slice_0']}, {1: 'test'})
            check.ms_run()
