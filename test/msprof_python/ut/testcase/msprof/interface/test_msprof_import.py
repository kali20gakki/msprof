import unittest
from argparse import Namespace
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ExportMode, ProfilingScene
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
            with mock.patch(NAMESPACE + '.LoadInfoManager.load_info'), \
                    mock.patch(NAMESPACE + '.get_path_dir', return_value=['host', 'device_1', 'device_2']), \
                    mock.patch(NAMESPACE + '.get_valid_sub_path', return_value='host'), \
                    mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
                    mock.patch('os.path.join', return_value=True), \
                    mock.patch('os.path.realpath', return_value='home\\process'), \
                    mock.patch('msinterface.msprof_c_interface.dump_device_data'), \
                    mock.patch(NAMESPACE + '.ImportCommand._start_parse'), \
                    mock.patch('os.listdir', return_value=['123']):
                ChipManager().chip_id = 5
                key = ImportCommand(args)
                key.process()
                with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=False), \
                        mock.patch(NAMESPACE + '.warn'), \
                        mock.patch('os.listdir', return_value=['123']):
                    key = ImportCommand(args)
                    key.process()

    def test_parse_unresolved_dirs(self):
        unresolved_dirs = {'pro_dir': ['result_dir']}
        args_dic = {"collection_path": "test", "cluster_flag": False}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.LoadInfoManager.load_info'), \
                mock.patch(NAMESPACE + '.get_path_dir', return_value=['host', 'device_1', 'device_2', 'device_3']), \
                mock.patch(NAMESPACE + '.get_valid_sub_path'), \
                mock.patch('common_func.msprof_common.prepare_log'), \
                mock.patch('common_func.config_mgr.ConfigMgr.is_ai_core_sample_based'), \
                mock.patch(NAMESPACE + '.ImportCommand.do_import'), \
                mock.patch('importlib.import_module'):
            InfoConfReader()._info_json = {"drvVersion": InfoConfReader().ALL_EXPORT_VERSION}
            InfoConfReader()._sample_json = {"devices": "5"}
            ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
            key = ImportCommand(args)
            key._parse_unresolved_dirs(unresolved_dirs)
            InfoConfReader()._info_json = {}
            InfoConfReader()._sample_json = {}


if __name__ == '__main__':
    unittest.main()
