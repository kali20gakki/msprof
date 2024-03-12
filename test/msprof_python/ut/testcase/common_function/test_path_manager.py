import os
import shutil
import unittest
from unittest import mock

from common_func.path_manager import PathManager

NAMESPACE = 'common_func.path_manager'
PROF_DIR = "PROF_1_TEST"
DEVICE_DIR = "device_0"
HOST_DIR = "host"



class TestPathManager(unittest.TestCase):

    def test_get_device_count_success_when_result_is_not_empty(self):
        with mock.patch('os.listdir', return_value=[DEVICE_DIR, 'device_1']):
            count = PathManager.get_device_count("PROF_1")
            self.assertEqual(count, 2)

    def test_del_summary_and_timeline_dir_success_when_result_dir_is_exist(self):
        shutil.rmtree(PROF_DIR, ignore_errors=True)
        os.mkdir(PROF_DIR, 0o777)
        os.mkdir(os.path.join(PROF_DIR, DEVICE_DIR), 0o777)
        os.mkdir(os.path.join(PROF_DIR, HOST_DIR), 0o777)
        os.mkdir(os.path.join(PROF_DIR, DEVICE_DIR, "summary"), 0o777)
        os.mkdir(os.path.join(PROF_DIR, HOST_DIR, "timeline"), 0o777)
        PathManager.del_summary_and_timeline_dir(PROF_DIR, [DEVICE_DIR, HOST_DIR])
        shutil.rmtree(PROF_DIR, ignore_errors=True)

