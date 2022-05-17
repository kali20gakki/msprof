import os
import shutil
import unittest

from common_func.constant import Constant
from common_func.file_manager import FileManager
from common_func.file_name_manager import FileNameManagerConstant

NAMESPACE = 'common_func.file_manager'


class TestFileManager(unittest.TestCase):

    def test_is_analyzed_data(self):
        os.mkdir("test_file_manager", 0o777)
        self.assertEqual(False, FileManager.is_analyzed_data("test_file_manager"))
        os.mkdir(os.path.join("test_file_manager", "data"), 0o777)
        self.assertEqual(False, FileManager.is_analyzed_data("test_file_manager"))
        with os.fdopen(os.open(os.path.join("test_file_manager/data", FileNameManagerConstant.ALL_FILE_TAG),
                               Constant.WRITE_FLAGS, Constant.WRITE_MODES), "w"):
            pass
        self.assertEqual(True, FileManager.is_analyzed_data("test_file_manager"))
        shutil.rmtree("test_file_manager")
