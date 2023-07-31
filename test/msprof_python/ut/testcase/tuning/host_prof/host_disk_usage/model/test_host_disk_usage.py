import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from host_prof.host_disk_usage.model.host_disk_usage import HostDiskUsage

NAMESPACE = 'host_prof.host_disk_usage.model.host_disk_usage'


class TsetHostDiskUsage(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_insert_disk_usage_data(self):
        disk_usage_info = {}
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostDiskUsage(self.result_dir)
            check.insert_disk_usage_data(disk_usage_info)

    def test_has_disk_usage_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist'):
            check = HostDiskUsage(self.result_dir)
            check.has_disk_usage_data()

    def test_get_disk_usage_data(self):
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        disk_info_list = ((0, 1, 2, 3, 4, 5, 6),)
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                        return_value=disk_info_list):
            check = HostDiskUsage(self.result_dir)
            result = check.get_disk_usage_data()
        self.assertEqual(result, {'data': [{'end': 0.001, 'start': 0.0, 'usage': 5}]})


if __name__ == '__main__':
    unittest.main()
