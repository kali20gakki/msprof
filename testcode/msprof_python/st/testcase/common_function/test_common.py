import unittest
from unittest import mock

from common_func.common import get_data_file_memory, check_free_memory
from common_func.constant import Constant
from common_func.db_manager import DBManager

NAMESPACE = 'common_func.common'


def test_get_data_file_memory():
    res_1 = get_data_file_memory(" ")
    unittest.TestCase().assertEqual(res_1, Constant.MAX_READ_FILE_BYTES)
    with mock.patch('os.listdir', return_value=["1"]), \
            mock.patch('os.path.getsize', return_value=1048576):
        res = get_data_file_memory("./")
        unittest.TestCase().assertEqual(res, 1)


def test_check_free_memory():
    with mock.patch(NAMESPACE + '.get_data_file_memory', return_value=1), \
            mock.patch(NAMESPACE + '.warn'), \
            mock.patch(NAMESPACE + '.get_free_memory', return_value=100):
        check_free_memory("./")
    with mock.patch(NAMESPACE + '.get_data_file_memory', return_value=1):
        check_free_memory("./")


def test_fetch_all_data():
    expect_res = []
    curs = mock.Mock()
    curs.execute.return_value = ValueError
    res = DBManager.fetch_all_data(curs, "")
    unittest.TestCase().assertEqual(expect_res, res)


if __name__ == '__main__':
    unittest.main()
