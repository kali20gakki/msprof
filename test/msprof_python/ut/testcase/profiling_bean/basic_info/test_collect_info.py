import unittest
from unittest import mock
from common_func.info_conf_reader import InfoConfReader
from profiling_bean.basic_info.collect_info import CollectInfo

NAMESPACE = 'profiling_bean.basic_info.collect_info'

info_json = {"clockMonotonicRaw": 10}
start_info = {"collectionTimeBegin": 0, "clockMonotonicRaw": 10}
end_info = {"collectionTimeEnd": 10}


class TestCollectInfo(unittest.TestCase):
    def test_run(self):
        with mock.patch(NAMESPACE + '.CollectInfo.get_collect_time'), \
                mock.patch(NAMESPACE + '.CollectInfo.get_project_size'):
            CollectInfo().run("")

    def test_get_collect_time_1(self):
        InfoConfReader()._info_json = {}
        InfoConfReader()._start_info = start_info
        InfoConfReader()._end_info = end_info
        CollectInfo().get_collect_time()

    def test_get_collect_time_2(self):
        InfoConfReader()._info_json = info_json
        InfoConfReader()._start_info = start_info
        InfoConfReader()._end_info = end_info
        CollectInfo().get_collect_time()

    def test_get_project_size(self):
        with mock.patch('os.path.getsize', return_value=1048576):
            CollectInfo().get_project_size(".")


if __name__ == '__main__':
    unittest.main()
