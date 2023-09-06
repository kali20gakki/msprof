import unittest
from argparse import Namespace
from unittest import mock

from msinterface.msprof_analyze import AnalyzeCommand

NAMESPACE = 'msinterface.msprof_analyze'


class TestAnalyzeCommand(unittest.TestCase):

    def test_analyze_command_clear_dir_with_unset_clear_mode(self):
        args_dic = {"collection_path": "test", "rule": "communication"}
        args = Namespace(**args_dic)
        test = AnalyzeCommand(args)
        test._clear_dir("")

    def test_analyze_command_clear_dir_with_set_clear_mode_false(self):
        args_dic = {"collection_path": "test", "rule": "communication", "clear_mode": False}
        args = Namespace(**args_dic)
        test = AnalyzeCommand(args)
        test._clear_dir("")

    def test_analyze_command_clear_dir_with_set_clear_mode_True(self):
        args_dic = {"collection_path": "test", "rule": "communication", "clear_mode": True}
        args = Namespace(**args_dic)
        test = AnalyzeCommand(args)
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['device_0', 'host', 'timeline']), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.check_dir_writable'), \
                mock.patch('shutil.rmtree'):
            test._clear_dir("")


if __name__ == '__main__':
    unittest.main()
