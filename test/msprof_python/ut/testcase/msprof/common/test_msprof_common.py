import os
import unittest
from unittest import mock
import shutil

import pytest
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_common import analyze_collect_data
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import get_info_by_key
from common_func.msprof_common import get_path_dir
from common_func.msprof_common import prepare_for_parse
from common_func.msprof_common import add_all_file_complete
from common_func.msprof_common import check_collection_dir
from common_func.msprof_exception import ProfException

from constant.constant import INFO_JSON

NAMESPACE = 'common_func.msprof_common'


def test_check_path_valid_error():
    with mock.patch(NAMESPACE + '.error'), \
            pytest.raises(ProfException) as err:
        path = ""
        is_output = True
        check_path_valid(path, is_output)
    unittest.TestCase().assertEqual(err.value.args, (1, ''))


def test_check_path_valid_1():
    path = "home\\common"
    is_output = 1
    with mock.patch('os.path.exists', return_value=False):
        with mock.patch('os.makedirs'), \
                mock.patch('os.chmod'), \
                mock.patch(NAMESPACE + '.check_dir_writable'):
            check_path_valid(path, is_output)
        with mock.patch('os.makedirs', side_effect=OSError), \
                mock.patch(NAMESPACE + '.error'), \
                pytest.raises(ProfException) as err:
            check_path_valid(path, is_output)
    unittest.TestCase().assertEqual(err.value.args, (2, ''))


def test_prepare_for_parse():
    output_path = 'home\\output'
    with mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value='home\\output'), \
            mock.patch(NAMESPACE + '.PathManager.get_log_dir', return_value='home\\output'), \
            mock.patch(NAMESPACE + '.check_path_valid'), \
            mock.patch(NAMESPACE + '.PathManager.get_collection_log_path', return_value=1), \
            mock.patch(NAMESPACE + '.check_file_writable'), \
            mock.patch(NAMESPACE + '.logging.basicConfig'):
        prepare_for_parse(output_path)


def test_analyze_collect_data():
    collect_path = './'
    sample_config = {"result_dir": './', "tag_id": '123', "host_id": '127.0.0.1'}
    with mock.patch('os.path.exists', return_value=True):
        with mock.patch('os.listdir', return_value=["1"]), \
                mock.patch(NAMESPACE + '.prepare_for_parse'), \
                mock.patch(NAMESPACE + '.print_info'), \
                mock.patch(NAMESPACE + '.check_collection_dir'), \
                mock.patch('os.path.basename', return_value='123'), \
                mock.patch(NAMESPACE + '.update_sample_json'), \
                mock.patch(NAMESPACE + '.AI.project_preparation'), \
                mock.patch(NAMESPACE + '.AI.import_control_flow'), \
                mock.patch(NAMESPACE + '.FileDispatch.dispatch_parser'), \
                mock.patch(NAMESPACE + '.files_chmod'), \
                mock.patch(NAMESPACE + '.add_all_file_complete'):
            analyze_collect_data(collect_path, sample_config)


def test_get_info_by_key():
    path = 'home\\key'
    key = 1
    with mock.patch(NAMESPACE + '.check_dir_readable'), \
            mock.patch('os.listdir', return_value=('info.json.0',)):
        with mock.patch(NAMESPACE + '.re.match', return_value=True):
            InfoConfReader()._info_json = INFO_JSON
            InfoConfReader.INFO_PATTERN = r"^info\.json\.?(\d?)$"
            result = get_info_by_key(path, key)
        unittest.TestCase().assertEqual(result, None)
        with mock.patch(NAMESPACE + '.re.match', return_value=False):
            InfoConfReader.INFO_PATTERN = r"^info\.json\.?(\d?)$"
            get_info_by_key(path, key)


def test_add_all_file_complete():
    collect_path = './'
    add_all_file_complete(collect_path)
    os.mkdir('data', 0o640)
    with mock.patch('os.makedirs', side_effect=OSError), \
            mock.patch(NAMESPACE + '.error'):
        add_all_file_complete(collect_path)
    add_all_file_complete(collect_path)
    shutil.rmtree('data')


def test_get_path_dir():
    path = 'home\\dir'
    with mock.patch('os.path.join', return_value='home\\dir'), \
            mock.patch('os.path.realpath', return_value='home\\dir_path'), \
            mock.patch('os.path.isdir', return_value=True), \
            mock.patch('os.listdir', return_value=[".profilerv", "HCCL_PROF"]):
        result = get_path_dir(path)
        unittest.TestCase().assertEqual(result, ['.profilerv'])
    with mock.patch('os.path.join', return_value='home\\dir'), \
            mock.patch('os.path.realpath', return_value='home\\dir_path'), \
            mock.patch('os.path.isdir', return_value=True), \
            mock.patch('os.listdir', return_value=[".profiler", "HCCL_PROF"]), \
            mock.patch(NAMESPACE + '.error'), \
            pytest.raises(ProfException) as err:
        get_path_dir(path)
        unittest.TestCase().assertEqual(err.value.args, (2,))


def test_check_collection_dir():
    collect_path = './'
    with mock.patch('os.path.exists', return_value=False), \
            mock.patch(NAMESPACE + '.error'),\
            pytest.raises(ProfException) as err:
        check_collection_dir(collect_path)
    unittest.TestCase().assertEqual(err.value.args, (4, ''))
    with mock.patch('os.path.exists', return_value=True), \
            mock.patch(NAMESPACE + '.check_dir_writable'), \
            mock.patch('common_func.common.get_data_file_memory', return_value=100), \
            mock.patch(NAMESPACE + '.PathManager.get_data_dir', return_value=["1"]):
        with mock.patch('os.listdir', return_value=False), \
                mock.patch(NAMESPACE + '.error'), \
                pytest.raises(ProfException) as err:
            check_collection_dir(collect_path)
            unittest.TestCase().assertEqual(err.value.args, (4, ''))


if __name__ == '__main__':
    unittest.main()
