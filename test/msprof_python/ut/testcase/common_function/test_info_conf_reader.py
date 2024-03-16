import unittest

import pytest

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException

NAMESPACE = 'common_func.info_conf_reader'


class TestInfoConfReader(unittest.TestCase):
    def tearDown(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {}
        info_reader._host_freq = None
        info_reader._local_time_offset = 0
        info_reader._start_info = {}
        info_reader._dev_cnt = 0.0

    def test_time_from_host_syscnt_should_return_original_time_when_host_freq_is_default(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        self.assertEqual(InfoConfReader().time_from_host_syscnt(15000000000), 15000000000.0)

    def test_time_from_host_syscnt_should_return_timestamp_when_host_freq_is_normal(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "3000"}]}
        self.assertEqual(InfoConfReader().time_from_host_syscnt(15000000000), 5000000000.0)

    def test_get_host_duration_should_return_original_duration_when_host_freq_is_default(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        self.assertEqual(InfoConfReader().get_host_duration(6000), 6000.0)

    def test_get_host_duration_should_return_computed_duration_when_host_freq_is_normal(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "3000"}]}
        self.assertEqual(InfoConfReader().get_host_duration(6000), 2000.0)

    def test_syscnt_from_dev_time_should_return_timestamp_when_host_freq_is_default(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        self.assertEqual(InfoConfReader().get_host_syscnt_from_dev_time(15000000000), 15000000000.0)

    def test_syscnt_from_dev_time_should_return_sys_count_duration_when_host_freq_is_normal(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "3000"}]}
        self.assertEqual(InfoConfReader().get_host_syscnt_from_dev_time(15000000000), 45000000000.0)

    def test_get_host_freq_should_return_host_freq_when_host_freq_not_None(self):
        InfoConfReader()._host_freq = 1000000000
        self.assertEqual(InfoConfReader().get_host_freq(), 1000000000)

    def test_get_host_freq_should_return_error_when_no_freq(self):
        InfoConfReader()._info_json = {'CPU': []}
        with pytest.raises(ProfException) as error:
            InfoConfReader().get_host_freq()
            self.assertEqual(error.value, 0)

    def test_get_host_freq_should_return_host_freq_when_host_freq_is_invalid(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "str"}]}
        self.assertEqual(InfoConfReader().get_host_freq(), 1000000000.0)

    def test_get_host_freq_should_return_host_freq_when_host_freq_is_normal(self):
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "3000"}]}
        self.assertEqual(InfoConfReader().get_host_freq(), 3000000000.0)

    def test_trans_into_local_time_should_return_local_time_when_time_fmt_is_not_default(self):
        InfoConfReader()._local_time_offset = 3000.0
        self.assertEqual(InfoConfReader().trans_into_local_time(3000000000.0, use_us=True), '3000003000.000')

    def test_get_dev_cnt_should_return_nano_second(self):
        InfoConfReader()._dev_cnt = 3000.000
        self.assertEqual(InfoConfReader().get_dev_cnt(), 3000000000000)


if __name__ == '__main__':
    unittest.main()
