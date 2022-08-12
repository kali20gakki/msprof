import unittest
from argparse import Namespace
from unittest import mock

from msinterface.msprof_import import ImportCommand

NAMESPACE = 'msinterface.msprof_import'


class TestImportCommand(unittest.TestCase):
    def test_do_import(self):
        result_dir = '123'
        args_dic = {"collection_path": "test", "cluster_flag": False}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value=True), \
                mock.patch(NAMESPACE + '.analyze_collect_data'):
            key = ImportCommand(args)
            key.do_import(result_dir)
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value=True), \
                mock.patch(NAMESPACE + '.analyze_collect_data'):
            key = ImportCommand(args)
            key.do_import(result_dir)

    def test_process(self):
        with mock.patch(NAMESPACE + '.check_path_valid'):
            args_dic = {"collection_path": "test", "cluster_flag": False}
            args = Namespace(**args_dic)
            with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
                    mock.patch(NAMESPACE + '.ImportCommand.do_import'), \
                    mock.patch(NAMESPACE + '.LoadInfoManager.load_info'), \
                    mock.patch('os.listdir', return_value=['123']):
                key = ImportCommand(args)
                key.process()
            with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=False), \
                    mock.patch(NAMESPACE + '.LoadInfoManager.load_info'), \
                    mock.patch(NAMESPACE + '.get_path_dir', return_value=[1, 2, 3]), \
                    mock.patch('os.path.join', return_value=True), \
                    mock.patch('os.path.realpath', return_value='home\\process'), \
                    mock.patch(NAMESPACE + '.DataCheckManager.process_check', return_value=True), \
                    mock.patch(NAMESPACE + '.ImportCommand.do_import'), \
                    mock.patch('os.listdir', return_value=['123']):
                key = ImportCommand(args)
                key.process()
                with mock.patch(NAMESPACE + '.DataCheckManager.process_check', return_value=False), \
                        mock.patch(NAMESPACE + '.warn'), \
                        mock.patch('os.listdir', return_value=['123']):
                    key = ImportCommand(args)
                    key.process()


if __name__ == '__main__':
    unittest.main()
