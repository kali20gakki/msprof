import os
import unittest
from unittest import mock
import shutil
from common_func.file_manager import FileManager
from common_func.file_name_manager import FileNameManagerConstant
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager

NAMESPACE = 'common_func.data_check_manager'


class TestDataCheckManager(unittest.TestCase):

    def test_check_file_size(self):
        check = DataCheckManager._check_file_size("a.done", "b")
        self.assertEqual(check, False)

        with mock.patch('os.path.realpath', return_value="a"), \
             mock.patch('os.path.getsize', return_value=0):
            check = DataCheckManager._check_file_size("a", "b")
        self.assertEqual(check, False)

        with mock.patch('os.path.realpath', return_value="a"), \
             mock.patch('os.path.getsize', return_value=10):
            check = DataCheckManager._check_file_size("a", "b")
        self.assertEqual(check, True)
