import unittest
from unittest import mock

from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from msparser.stars.acsq_task_parser import AcsqTaskParser
from profiling_bean.stars.acsq_task import AcsqTask

NAMESPACE = 'msparser.stars.acsq_task_parser'


class TestAcsqTaskParser(unittest.TestCase):
    def test_preprocess_data(self):
        result_dir = ''
        db = ''
        table_list = ['']
        with mock.patch(NAMESPACE + '.AcsqTaskParser.get_task_time', return_value=[1, 2]):
            key = AcsqTaskParser(result_dir, db, table_list)
            key.preprocess_data()

    def test_get_task_time_should_return_2_matched_task_and_no_remaining_when_no_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        key._data_list = (AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2), AcsqTask(args_end_2))
        ret = key.get_task_time()
        expect_ret = (
            [
                [3, 4, 0, 'AI_CORE', 1120.0, 1160.0, 40.0],
                [67, 7, 0, 'AI_CORE', 1780.0, 1820.0, 40.0]
            ],
            []
        )
        self.assertEqual(ret, expect_ret)

    def test_get_task_time_should_return_2_matched_task_and_0_remaining_when_1_end_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        args_end_3 = [1, 5, 67, 7, 93, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        key._data_list = (
            AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2),
            AcsqTask(args_end_2), AcsqTask(args_end_3)
        )
        ret = key.get_task_time()
        expect_ret = (
            [
                [3, 4, 0, 'AI_CORE', 1120.0, 1160.0, 40.0],
                [67, 7, 0, 'AI_CORE', 1780.0, 1820.0, 40.0]
            ],
            []
        )
        self.assertEqual(ret, expect_ret)

    def test_get_task_time_should_return_2_matched_task_and_1_remaining_when_1_start_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        args_begin_3 = [0, 5, 67, 1000, 93, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        key._data_list = (
            AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2),
            AcsqTask(args_end_2), AcsqTask(args_begin_3)
        )
        ret = key.get_task_time()
        expect_ret = [
            [3, 4, 0, 'AI_CORE', 1120.0, 1160.0, 40.0],
            [67, 7, 0, 'AI_CORE', 1780.0, 1820.0, 40.0]
        ]
        self.assertEqual(ret[0], expect_ret)
        self.assertEqual(len(ret[1]), 1)
        self.assertEqual(ret[1][0].task_id, 1000)

    def test_get_task_time_should_return_2_matched_task_and_0_remaining_when_2_start_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        args_begin_3 = [0, 5, 67, 1000, 93, 47, 8, 99]
        args_begin_4 = [0, 5, 67, 1000, 95, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        key._data_list = (
            AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2),
            AcsqTask(args_end_2), AcsqTask(args_begin_3), AcsqTask(args_begin_4)
        )
        ret = key.get_task_time()
        expect_ret = (
            [
                [3, 4, 0, 'AI_CORE', 1120.0, 1160.0, 40.0],
                [67, 7, 0, 'AI_CORE', 1780.0, 1820.0, 40.0]
            ],
            []
        )
        self.assertEqual(ret, expect_ret)

    def test_get_task_time_should_return_2_matched_task_and_0_remaining_when_2_start_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        args_begin_3 = [0, 5, 67, 1000, 93, 47, 8, 99]
        args_begin_4 = [0, 5, 67, 1000, 95, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        key._data_list = (
            AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2),
            AcsqTask(args_end_2), AcsqTask(args_begin_3), AcsqTask(args_begin_4)
        )
        ret = key.get_task_time()
        expect_ret = (
            [
                [3, 4, 0, 'AI_CORE', 1120.0, 1160.0, 40.0],
                [67, 7, 0, 'AI_CORE', 1780.0, 1820.0, 40.0]
            ],
            []
        )
        self.assertEqual(ret, expect_ret)

    def test_get_task_time_should_return_2_matched_task_and_0_remaining_when_end_less_than_start(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_end_0 = [1, 5, 67, 7, 53, 47, 8, 99]
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        args_begin_3 = [0, 5, 67, 1000, 93, 47, 8, 99]
        args_begin_4 = [0, 5, 67, 1000, 95, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        key._data_list = (
            AcsqTask(args_end_0), AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2),
            AcsqTask(args_end_2), AcsqTask(args_begin_3), AcsqTask(args_begin_4)
        )
        ret = key.get_task_time()
        expect_ret = (
            [
                [3, 4, 0, 'AI_CORE', 1120.0, 1160.0, 40.0],
                [67, 7, 0, 'AI_CORE', 1780.0, 1820.0, 40.0]
            ],
            []
        )
        self.assertEqual(ret, expect_ret)

    def test_flush(self):
        result_dir = ''
        db = ''
        table_list = ['']
        key = AcsqTaskParser(result_dir, db, table_list)
        key.flush()
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        key._data_list = [
            AcsqTask(args_begin_1), AcsqTask(args_end_1)
        ]
        with mock.patch(NAMESPACE + '.AcsqTaskModel.init'), \
                mock.patch(NAMESPACE + ".AcsqTaskParser.preprocess_data"), \
                mock.patch(NAMESPACE + ".AcsqTaskModel.flush"), \
                mock.patch(NAMESPACE + ".AcsqTaskModel.finalize"):
            key.flush()


if __name__ == '__main__':
    unittest.main()
