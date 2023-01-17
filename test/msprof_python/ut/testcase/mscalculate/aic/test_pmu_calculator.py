import collections
import sqlite3
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from mscalculate.aic.pmu_calculator import PmuCalculator
from sqlite.db_manager import DBManager

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'mscalculate.aic.pmu_calculator'


class TestPmuCalculator(unittest.TestCase):
    GeDataBean = collections.namedtuple('ge_data', ['task_type', 'task_id', 'stream_id', 'block_dim', 'mix_block_dim'])

    def test_init_param(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.PmuCalculator.get_block_dim_from_ge'), \
                mock.patch(NAMESPACE + '.PmuCalculator.get_config_params'):
            check = PmuCalculator()
            check.sample_config = {'result_dir': 'test', 'device_id': '0', 'iter_id': 1, 'job_id': 'job_default'}
            check.init_params()

    def test_get_block_dim_from_ge(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.PmuCalculator.get_db_path', return_value=''), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                mock.patch('common_func.utils.Utils.get_scene', return_value="single_op"), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)):
            key = PmuCalculator()
            key._core_num_dict = {'aic': 30, 'aiv': 0}
            key._block_dims = {'block_dim': {'0-0': [20]}}
            key._freq = 1500
            key.get_block_dim_from_ge()
        ProfilingScene().init('test')
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.PmuCalculator.get_db_path', return_value=''), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                           return_value=[self.GeDataBean('', 0, 0, 0, 0), self.GeDataBean("AI_CORE", 0, 0, 20, 40),
                                         self.GeDataBean('MIX_AIC', 0, 0, 20, 40)]), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                           return_value=[[1, 1]]), \
                mock.patch('common_func.utils.Utils.get_scene', return_value="step_info"), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)):
            key = PmuCalculator()
            key._block_dims = {'block_dim': {'0-0': [20]}, 'mix_block_dim': {'0-0': [20]}}
            key._core_num_dict = {'aic': 30, 'aiv': 0}
            key._freq = 1500
            key.get_block_dim_from_ge()

    def test_get_db_path(self):
        db_name = '111.db'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('os.path.join', return_value='test\\test.text'):
            key = PmuCalculator()
            result = key.get_db_path(db_name)
        self.assertEqual(result, 'test\\test.text')

    def test_get_current_block(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('os.path.join', return_value='test\\test.text'):
            key = PmuCalculator()
            key._block_dims = {'block_dim': {'0-0': [20]}}
            result = key._get_current_block('block_dim', self.GeDataBean('', 0, 0, 20, 40))
            self.assertEqual(result, 20)

    def test_get_config_params(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('os.listdir', return_value=['info.json.0']):
            key = PmuCalculator()
            InfoConfReader()._info_json = {'DeviceInfo': [{'ai_core_num': 8, 'aiv_num': 8, 'aic_frequency': 1500}]}
            key._block_dims = {'block_dim': {'0-0': [20]}}
            key._core_num_dict = {'aic': 30, 'aiv': 0}
            key._block_dims = {'block_dim': {'2-2': [22, 22]}}
            key._freq = 1500
            key.get_config_params()


if __name__ == '__main__':
    unittest.main()
