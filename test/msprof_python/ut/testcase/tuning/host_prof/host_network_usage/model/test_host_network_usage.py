import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from host_prof.host_network_usage.model.host_network_usage import HostNetworkUsage

NAMESPACE = 'host_prof.host_network_usage.model.host_network_usage'


class TsetHostNetworkUsage(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_flush_data(self):
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostNetworkUsage(self.result_dir)
            check.cache_data = {}
            check.flush_data()

    def test_has_network_usage_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist'):
            check = HostNetworkUsage(self.result_dir)
            check.has_network_usage_data()

    def test_get_network_usage_data(self):
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        disk_info_list = ((0, 1, 2, 3, 4, 5, 6),)
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                        return_value=disk_info_list):
            check = HostNetworkUsage(self.result_dir)
            result = check.get_network_usage_data()
        self.assertEqual(result, {'data': [{'end': 0.001, 'start': 0.0, 'usage': 2}]})


if __name__ == '__main__':
    unittest.main()
