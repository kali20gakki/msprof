import unittest
from unittest import mock
import pytest
from common_func.msprof_exception import ProfException
from tuning.tuning_control import TuningControl


class TestTuningControl(unittest.TestCase):

    def test_dump_to_file(self):
        with mock.patch('tuning.tuning_control.PathManager.get_summary_dir', return_value='test'),\
                mock.patch('tuning.tuning_control.check_path_valid', side_effect=ProfException(1)):
            try:
                TuningControl().dump_to_file('test', 0)
            except ProfException as ex:
                self.assertEqual(ex.code, ProfException.PROF_SYSTEM_EXIT)
