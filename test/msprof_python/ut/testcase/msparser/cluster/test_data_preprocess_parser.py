import os
import unittest
from unittest import mock

import pytest

from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from common_func.platform.chip_manager import ChipManager
from constant.constant import clear_dt_project
from msparser.cluster.data_preprocess_parser import DataPreprocessParser
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBOpen

NAMESPACE = 'msparser.cluster.data_preprocess_parser'


class DataList:
    def __init__(self, *args):
        self.op_type = args[1]
        self.total_fops = args[0]
        self.total_time = args[2]
        self.rank_id = args[3]
        self.dir_name = args[4]


class TestDataPreprocessParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_DataPreprocessParser')
    params = {'model_id': 1, 'iteration_id': 1, 'npu_id': 1,
              'collection_path': os.path.join(DIR_PATH, "PROF1", "device_0", "sqlite")}

    def setUp(self):
        os.makedirs(os.path.join(self.DIR_PATH, "PROF1", "device_0", "sqlite"))

    def tearDown(self):
        clear_dt_project(self.DIR_PATH)

    def test_get_data_queue_data(self):
        db_path = os.path.join(self.DIR_PATH, "PROF1", "device_0", "sqlite", "data_preprocess.db")
        with DBOpen(DBNameConstant.DB_CLUSTER_DATA_PREPROCESS) as db_connect:
            with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=db_path), \
                    mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                               return_value=(False, False)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch('msmodel.ai_cpu.data_queue_model.DataQueueModel.check_db', return_value=True), \
                    mock.patch('msmodel.ai_cpu.data_queue_model.DataQueueModel.check_table', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
                check = DataPreprocessParser({'model_id': 1, 'iter_id': 1, 'rank_id': 1})
                self.assertEqual(check.get_data_queue_data(), [])
            with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=db_path), \
                    mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                               return_value=(db_connect.db_conn, db_connect.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch('msmodel.ai_cpu.data_queue_model.DataQueueModel.check_db', return_value=False), \
                    mock.patch('msmodel.ai_cpu.data_queue_model.DataQueueModel.check_table', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=10):
                check = DataPreprocessParser({'model_id': 1, 'iter_id': 1, 'rank_id': 1})
                ChipManager().chip_id = ChipModel.CHIP_V4_1_0
                self.assertEqual(check.get_data_queue_data(), [])

    def test_run(self):
        with mock.patch('common_func.utils.Utils.is_step_scene', side_effect=(False, True)), \
                mock.patch('common_func.utils.Utils.is_single_op_graph_mix', return_value=False), \
                mock.patch(NAMESPACE + '.DataPreprocessParser.calculate'):
            check = DataPreprocessParser(self.params)
            check.process()
        with mock.patch('common_func.utils.Utils.is_step_scene', side_effect=(False, True)), \
                mock.patch('common_func.utils.Utils.is_single_op_graph_mix', return_value=False), \
                mock.patch(NAMESPACE + '.DataPreprocessParser.calculate'):
            check = DataPreprocessParser({})
            check.process()

    def test_calculate_queue_data(self):
        queue_list = [('GetNext_dequeue_wait', 108, 65347257104.0, 65347257142.0, 38.0),
                      ('GetNext_dequeue_wait', 107, 65347261988.0, 65347262033.0, 45.0),
                      ('GetNext_dequeue_wait', 106, 65347263085.0, 65347263098.0, 13.0),
                      ('GetNext_dequeue_wait', 107, 65347268208.0, 65347268224.0, 16.0),
                      ('GetNext_dequeue_wait', 107, 65347268208.0, 65347268224.0, 16.0)]
        trace_data = {1: [0, 43423945119245], 2: [43423945119245, 43428275833234]}
        check = DataPreprocessParser(self.params)
        check.calculate_queue_data(queue_list, trace_data)

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.DataPreprocessParser.check_id_valid', return_value=[(10,)]):
            with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data',
                            side_effect=(True, False, False)), \
                    mock.patch('os.path.join', return_value=True), \
                    mock.patch('os.path.realpath', return_value='home\\process'), \
                    mock.patch(NAMESPACE + '.check_path_valid'), \
                    mock.patch('os.listdir', return_value=['123', '456', 'timeline']), \
                    mock.patch(NAMESPACE + '.DataPreprocessParser.query_data_queue_data', return_value=[1, 2]):
                check = DataPreprocessParser(self.params)

                check.calculate()
        with mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.DataPreprocessParser.check_id_valid', return_value=False):
            with pytest.raises(ProfException) as err:
                check = DataPreprocessParser(self.params)

                check.calculate()
        with mock.patch(NAMESPACE + '.DataPreprocessParser.check_id_valid', return_value=[(10,)]):
            with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data',
                            side_effect=(False,)), \
                    mock.patch(NAMESPACE + '.check_path_valid'):
                with pytest.raises(ProfException) as err:
                    check = DataPreprocessParser(self.params)

                    check.calculate()

    def test_storage_data(self):
        with mock.patch(NAMESPACE + '.DataPreprocessParser.get_cluster_path', return_value=self.DIR_PATH), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('builtins.open', mock.mock_open(read_data='')), \
                mock.patch('os.fdopen', side_effect=OSError), \
                mock.patch('os.chmod'), \
                mock.patch('os.remove'):
            with pytest.raises(ProfException) as err:
                check = DataPreprocessParser(self.params)
                check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
                check.storage_data({'total_info': {'step_count': 469, 'empty_queue': 0,
                                                   'total_time': 37856.0, 'avg_time': 80.7164}})

    def test_query_data_queue_data(self):
        with mock.patch(NAMESPACE + '.DataPreprocessParser.get_data_queue_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DataPreprocessParser.get_step_trace_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DataPreprocessParser.calculate_queue_data', return_value=[]):
            with pytest.raises(ProfException) as err:
                check = DataPreprocessParser(self.params)
                check.query_data_queue_data()
        with mock.patch(NAMESPACE + '.DataPreprocessParser.get_data_queue_data',
                        return_value=[('GetNext_dequeue_wait', 108, 65347257104.0, 65347257142.0, 38.0),
                                      ('GetNext_dequeue_wait', 108, 65347257104.0, 65347257142.0, 38.0)]), \
                mock.patch(NAMESPACE + '.DataPreprocessParser.get_step_trace_data',
                           return_value={1: [0, 43423945119245]}):
            check = DataPreprocessParser(self.params)
            check.sample_config = {'ai_core_metrics': ''}
            check.query_data_queue_data()
        with mock.patch(NAMESPACE + '.DataPreprocessParser.get_data_queue_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DataPreprocessParser.get_step_trace_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DataPreprocessParser.calculate_queue_data',
                           return_value=[(1, 2, 3, 4, 5, 6)]):
            with pytest.raises(ProfException) as err:
                check = DataPreprocessParser(self.params)
                check.query_data_queue_data()

    def test_get_cluster_path(self):
        with mock.patch('os.path.realpath', return_value='test\\query\\test\\test'), \
                mock.patch("os.path.exists", return_value=True):
            check = DataPreprocessParser(self.params)
            result = check.get_cluster_path('test\\test')
            self.assertEqual(result, 'test\\query\\test\\test')

    def test_check_id_valid(self):
        with DBOpen(DBNameConstant.DB_CLUSTER_RANK) as db_connect:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_connect.db_conn, db_connect.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                    mock.patch(NAMESPACE + '.DBManager.destroy_db_connect', return_value=[]):
                check = DataPreprocessParser(self.params)
                check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
                result = check.check_id_valid()
                self.assertFalse(result)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_connect.db_conn, db_connect.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                               return_value=[DataList(1, 2.3, 'test', 5, 'test')]), \
                    mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                check = DataPreprocessParser(self.params)
                check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
                result = check.check_id_valid()
                self.assertTrue(result)


if __name__ == '__main__':
    unittest.main()
