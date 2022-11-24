import sqlite3
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from analyzer.update_aicore import UpdateAICoreData
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJsonReaderManager
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo
from constant.constant import CONFIG
from sqlite.db_manager import DBManager

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'analyzer.update_aicore'


class TestUpdateAICoreData(unittest.TestCase):

    def test_run(self):
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__init_param', return_value=False):
            key = UpdateAICoreData(CONFIG)
            key.run()
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__init_param', return_value=True):
            with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__check_required_tables_exist',
                            return_value=False), \
                    mock.patch(NAMESPACE + '.logging.warning'):
                key = UpdateAICoreData(CONFIG)
                key.run()
            with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__check_required_tables_exist',
                            return_value=True), \
                    mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__update_ai_core_data'):
                key = UpdateAICoreData(CONFIG)
                key.run()
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__init_param', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            key = UpdateAICoreData(CONFIG)
            key.run()

    def test_run1(self):
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__init_param', return_value=True), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__check_required_tables_exist',
                           return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.UpdateAICoreData.get_db_path', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.logging.warning'):
            ProfilingScene().init("")
            with mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                            return_value=[]), \
                    mock.patch('common_func.utils.Utils.get_scene', return_value="single_op"):
                key = UpdateAICoreData(CONFIG)
                key.sql_dir = 'test'
                key.run()
            ProfilingScene().init("")
            with mock.patch(NAMESPACE + '.MsprofIteration.get_iter_list_with_index_and_model',
                            return_value=[[1, 1]]), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                               return_value=[]), \
                    mock.patch('common_func.utils.Utils.get_scene', return_value="step_info"):
                ProfilingScene().init("")
                key = UpdateAICoreData(CONFIG)
                key.sql_dir = 'test'
                key.run()

    def test_init_param(self):
        with mock.patch('os.path.exists', return_value=False):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__init_param()
        self.assertEqual(result, False)
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=True):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__init_param()
        self.assertEqual(result, True)

    def test_check_required_tables_exist(self):
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__check_ge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__check_ai_core_table_exist',
                           return_value=True):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__check_required_tables_exist()
        self.assertEqual(result, True and True)

    def test_check_ge_table_exist(self):
        with mock.patch(NAMESPACE + '.UpdateAICoreData.get_db_path', return_value='ge_task_data'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__check_ge_table_exist()
        self.assertEqual(result, True)

    def test_check_ai_core_table_exist(self):
        with mock.patch(NAMESPACE + '.UpdateAICoreData.get_db_path', return_value='MetricSummary'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__check_ai_core_table_exist()
        self.assertEqual(result, True)

    def test_get_db_path(self):
        db_name = '111.db'
        with mock.patch('os.path.join', return_value='test\\test.text'):
            key = UpdateAICoreData(CONFIG)
            result = key.get_db_path(db_name)
        self.assertEqual(result, 'test\\test.text')

    def test_update_aicore_data(self):
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_block_dim_from_ge', return_value=False), \
                mock.patch(NAMESPACE + '.logging.warning'):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__update_ai_core_data()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_block_dim_from_ge', return_value=True):
            with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_config_params', return_value=(0, 0)), \
                    mock.patch(NAMESPACE + '.logging.error'):
                key = UpdateAICoreData(CONFIG)
                result = key._UpdateAICoreData__update_ai_core_data()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_config_params', return_value=(1, 1)), \
                    mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__update_ai_core_db', return_value=True):
                key = UpdateAICoreData(CONFIG)
                result = key._UpdateAICoreData__update_ai_core_data()
            self.assertEqual(result, None)

    def test_get_aicore_data(self):
        db_manager = DBManager()
        res = db_manager.create_table("aicore.db")
        table_name = 'aicore'
        res[1].execute('create table if not exists aicore(test integer, name txt )')
        with mock.patch(NAMESPACE + '.DBManager.get_filtered_table_headers', return_value=['test']):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__get_ai_core_data(res[1], table_name)
        self.assertEqual(result, (['test'], []))
        res[1].execute("drop table aicore")
        db_manager.destroy(res)

    def test_get_col_indexes_to_cal_total_if(self):
        headers = ["task_id", "stream_id", "total_cycles"]
        key = UpdateAICoreData(CONFIG)
        result = key._UpdateAICoreData__get_col_indexes_to_cal_total(headers)
        self.assertEqual(result, (0, 1, 2, None))
        headers = ["task_id", "stream_id", "aic_total_cycles", "aiv_total_cycles"]
        key = UpdateAICoreData(CONFIG)
        result = key._UpdateAICoreData__get_col_indexes_to_cal_total(headers)
        self.assertEqual(result, (0, 1, 2, 3))

    def test_add_pipe_time_for(self):
        headers = ['vec_ratio', '123']
        ai_core_data = [[0], [1], [2]]
        key = UpdateAICoreData(CONFIG)
        result = key._UpdateAICoreData__add_pipe_time(headers, ai_core_data)
        self.assertEqual(result, (['vec_time', 'vec_ratio', '123'], [[0, 0], [1, 1], [4, 2]]))

    def test_add_pipe_time_if_not(self):
        headers = []
        ai_core_data = [[3], [4], [5]]
        key = UpdateAICoreData(CONFIG)
        result = key._UpdateAICoreData__add_pipe_time(headers, ai_core_data)
        self.assertEqual(result, ([], [[3], [4], [5]]))

    def test_update_aicore(self):
        create_sql = "CREATE TABLE IF NOT EXISTS hwts_sys_cnt_range (device_id INTEGER)"
        insert_sql = "insert into {} values (?)".format('hwts_sys_cnt_range')
        data = ('1',)
        db_manager = DBManager()
        res = db_manager.create_table("hwts-rec.db", create_sql, insert_sql, data)
        headers = ['1', '2', '3']
        table_name = 'hwts_sys_cnt_range'
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_create_sql_str',
                        return_value='hwts_sys_cnt_range'), \
                mock.patch(NAMESPACE + '.DBManager.insert_data_into_table', return_value=True):
            key = UpdateAICoreData(CONFIG)
            key._UpdateAICoreData__update_ai_core(res[0], headers, data, table_name)
        res[1].execute("drop table hwts_sys_cnt_range")
        db_manager.destroy(res)

    def test_get_create_sql_str(self):
        create_sql = "CREATE TABLE IF NOT EXISTS step_trace_data ( index_id INT,model_id INT,step_start," \
                     " step_end, iter_id INT default 1, ai_core_num INT default 0)"
        insert_sql = "insert into {} values (?,?,?,?,?,?)".format('step_trace_data')
        data = ((1, 1, 2, 3, 4, 5),)
        db_manager = DBManager()
        res = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        curs = res[1]
        headers = ['1', '2', '3']
        table_name = 'step_trace_data'
        with mock.patch(NAMESPACE + '.DBManager.get_table_info', return_value=[]):
            key = UpdateAICoreData(CONFIG)
            key._UpdateAICoreData__get_create_sql_str(curs, headers, table_name)
        res[1].execute("drop table step_trace_data")
        db_manager.destroy(res)


if __name__ == '__main__':
    unittest.main()
