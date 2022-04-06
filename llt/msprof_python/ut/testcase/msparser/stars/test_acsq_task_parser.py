import unittest

from unittest import mock

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

    def test_get_task_time(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin = [0, 2, 3, 4, 56, 7, 89, 9]
        args_begin_1 = [1, 2, 3, 4, 56, 7, 89, 9]
        args_end = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_1 = [1, 5, 67, 7, 89, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        key._data_list = (AcsqTask(args_begin), AcsqTask(args_end), AcsqTask(args_begin_1), AcsqTask(args_end_1))
        ret = key.get_task_time()
        self.assertEqual(ret, [[3, 4, 0, 'FFTS_SQE', 56, 56, 0], [67, 7, 0, 'FFTS_SQE', 89, 89, 0]])


if __name__ == '__main__':
    unittest.main()
