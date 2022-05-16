import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.info_conf_reader import InfoConfReader
from sqlite.db_manager import DBManager
import sqlite3

from analyzer.update_aicore import UpdateAICoreData
from constant.constant import CONFIG, INFO_JSON


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

    def test_get_block_dim_from_ge(self):
        create_sql = "create table IF NOT EXISTS ge_task_data (device_id INTEGER,model_name TEXT,model_id INTEGER," \
                     "op_name TEXT,stream_id INTEGER,task_id INTEGER,block_dim INTEGER,op_state TEXT,task_type TEXT," \
                     "op_type TEXT,iter_id INTEGER,input_count INTEGER,input_formats TEXT,input_data_types TEXT," \
                     "input_shapes TEXT,output_count INTEGER,output_formats TEXT,output_data_types TEXT," \
                     "output_shapes TEXT)"
        select_sql = "select task_id, stream_id, block_dim from ge_task_data " \
                     "where (iter_id=0 or iter_id=?) " \
                     "and task_type='AI_CORE'"
        db_manager = DBManager()
        res = db_manager.create_table("ge_info.db")
        res[1].execute(create_sql)
        with mock.patch(NAMESPACE + '.UpdateAICoreData.get_db_path', return_value='ge_info.db'):
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                key = UpdateAICoreData(CONFIG)
                result = key._UpdateAICoreData__get_block_dim_from_ge()
            self.assertEqual(result, {})
            db_manager = DBManager()
            res = db_manager.create_table("ge_info.db")
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
                with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=res), \
                        mock.patch('os.path.join', return_value=True), \
                        mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_block_dim_sql',
                                   return_value=(select_sql, (1,))):
                    key = UpdateAICoreData(CONFIG)
                    result = key._UpdateAICoreData__get_block_dim_from_ge()
                self.assertEqual(result, {})
                db_manager = DBManager()
                res = db_manager.create_table("ge_info.db")
                res[1].execute("insert into ge_task_data values (0, 'resnet50', 1, 'trans_TransData_0', "
                               "5, 3, 2, 'static', 'AI_CORE', 'TransData', 0, 1, 'NCHW','DT_FLOAT16', "
                               "'1,3,224,224', 1, 'NC1HWC0', 'DT_FLOAT16', '1,1,224,224,16')")
                with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=res), \
                     mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                     mock.patch('os.path.join', return_value=True):
                    with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_block_dim_sql',
                                    return_value=(select_sql, (1,))):
                        key = UpdateAICoreData(CONFIG)
                        result = key._UpdateAICoreData__get_block_dim_from_ge()
                    self.assertEqual(result, {'3-5': [2]})
        db_manager.destroy(res)

    # def test_get_block_dim_sql(self): XXX
    #     curs = 1
    #     create_sql = "create table IF NOT EXISTS ge_task_data (device_id INTEGER,model_name TEXT,model_id INTEGER," \
    #                  "op_name TEXT,stream_id INTEGER,task_id INTEGER,block_dim INTEGER,op_state TEXT,task_type TEXT," \
    #                  "op_type TEXT,iter_id INTEGER,input_count INTEGER,input_formats TEXT,input_data_types TEXT," \
    #                  "input_shapes TEXT,output_count INTEGER,output_formats TEXT,output_data_types TEXT," \
    #                  "output_shapes TEXT)"
    #     db_manager = DBManager()
    #     res = db_manager.create_table("ge_info.db")
    #     res[1].execute(create_sql)
    #     res[1].execute("insert into ge_task_data values (0, 'resnet50', 1, 'trans_TransData_0', "
    #                    "5, 3, 2, 'static', '{2}', 'TransData', 0, 1, 'NCHW','DT_FLOAT16', "
    #                    "'1,3,224,224', 1, 'NC1HWC0', 'DT_FLOAT16', '1,1,224,224,16')")
    #     select_sql = "select task_id, stream_id, block_dim from ge_task_data " \
    #                  "where (iter_id=0 or iter_id=1) " \
    #                  "and task_type='{2}'"
    #     ProfilingScene().init('test')
    #     with mock.patch(NAMESPACE + '.DBManager.get_table_headers', return_value=select_sql), \
    #             mock.patch('common_func.utils.Utils.get_scene', return_value='step_info'):
    #         key = UpdateAICoreData(CONFIG)
    #         result = key._UpdateAICoreData__get_block_dim_sql(curs)
    #     self.assertEqual(result[1], (1,))
    #     res[1].execute("drop table ge_task_data")
    #     res[0].commit()
    #     db_manager.destroy(res)

    def test_get_config_params(self):
        with mock.patch(NAMESPACE + '.FileManager.is_info_json_file', return_value='111'):
            key = UpdateAICoreData(CONFIG)
            InfoConfReader()._info_json = {'deviceinfo': None}
            result = key._UpdateAICoreData__get_config_params()
        self.assertEqual(result, (0, 0))
        with mock.patch(NAMESPACE + '.FileManager.is_info_json_file', return_value='111'):
            key = UpdateAICoreData(CONFIG)
            InfoConfReader()._info_json = INFO_JSON
            result = key._UpdateAICoreData__get_config_params()
        self.assertEqual(result, (0, 0)) # (8, 1150000000.0) XXX

    def test_update_aicore_db(self):
        block_dims = '123'
        core_num = 1
        freq = 1000
        tables = ['MetricSummary']
        create_sql = "create table IF NOT EXISTS MetricSummary ([total_time] numeric,[total_cycles] numeric," \
                     "[vec_time] numeric,[vec_ratio] numeric,[mac_time] numeric,[mac_ratio] numeric," \
                     "[scalar_time] numeric,[scalar_ratio] numeric,[mte1_time] numeric,[mte1_ratio] numeric," \
                     "[mte2_time] numeric,[mte2_ratio] numeric,[mte3_time] numeric,[mte3_ratio] numeric," \
                     "[icache_miss_rate] numeric,[device_id] INT,[task_id] INT,[stream_id] INT,[index_id] INT," \
                     "[model_id] INT)"
        data = ((0.0426514705882353, 58006, 0.019154, 0.449091473295866, 0, 0, 0.000387, 0.00906802744543668,
                 0, 0, 0.010296, 0.241388821846016, 0.019614, 0.459866220735786, 0.0535714285714286, 0, 3, 5, 1, 1),)
        insert_sql = "insert into {0} values ({value})".format("MetricSummary", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        with mock.patch(NAMESPACE + '.UpdateAICoreData.get_db_path', return_value='runtime.db'):
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                    mock.patch(NAMESPACE + '.logging.error'):
                key = UpdateAICoreData(CONFIG)
                key._UpdateAICoreData__update_ai_core_db(block_dims, core_num, freq)
                res = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=res):
                with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__update_ai_core_table',
                                   return_value=True):
                    key = UpdateAICoreData(CONFIG)
                    key._UpdateAICoreData__update_ai_core_db(block_dims, core_num, freq)
                with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__update_ai_core_table', side_effect=sqlite3.DatabaseError), \
                        mock.patch(NAMESPACE + '.logging.error'):
                    key = UpdateAICoreData(CONFIG)
                    key._UpdateAICoreData__update_ai_core_db(block_dims, core_num, freq)
        res = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        res[1].execute("drop table MetricSummary")
        res[0].commit()
        db_manager.destroy(res)

    def test_update_aicore_table(self):
        headers = ['Name', 'Type', 'Start Time', 'Process ID', 'Thread ID']
        db_manager = DBManager()
        res = db_manager.create_table("runtime.db")
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_ai_core_data',
                        return_value=(None, None)):
            key = UpdateAICoreData(CONFIG)
            key._UpdateAICoreData__update_ai_core_table(res[0], "MetricSummary", '123', 1, 1000)
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_ai_core_data',
                        return_value=(headers, [1])), \
                mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__cal_total', return_value=[1]), \
                mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__add_pipe_time',
                           return_value=(headers, [1])), \
                mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__update_ai_core'):
            key = UpdateAICoreData(CONFIG)
            key._UpdateAICoreData__update_ai_core_table(res[0], "MetricSummary", '123', 1, 1000)
        db_manager.destroy(res)

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

    def test_cal_total(self):
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_col_indexes_to_cal_total',
                        return_value=(-1, -1, -1)), \
                mock.patch(NAMESPACE + '.logging.error'):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__cal_total(['111', '222', '333'], ([4, 5, 6], [7, 8, 9]),
                                                      {'block_dims': 1}, 0, '100')
        self.assertEqual(result, [])
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_col_indexes_to_cal_total',
                        return_value=(1, 1, 1)):
            with mock.patch(NAMESPACE + '.UpdateAICoreData._get_current_block', return_value=1):
                key = UpdateAICoreData(CONFIG)
                key._UpdateAICoreData__cal_total(['111', '222', '333'], ([4, 5, 6], [7, 8, 9]),
                                                 {'block_dims': 1}, 0, '100')
            with mock.patch(NAMESPACE + '.UpdateAICoreData._get_current_block', return_value=1):
                key = UpdateAICoreData(CONFIG)
                key._UpdateAICoreData__cal_total(['111', '222', '333'], ([4, 5, 6], [7, 8, 9]),
                                                 {'block_dims': 1}, 1, '100')
        with mock.patch(NAMESPACE + '.UpdateAICoreData._UpdateAICoreData__get_col_indexes_to_cal_total',
                        return_value=(1, 2, 3)):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__cal_total(['111', '222', '333'], ([4, 5, 6], [7, 8, 9]),
                                                      {'block_dims': 1}, 0, '100')
        self.assertEqual(result, [[0, 4, 5, 6], [0, 7, 8, 9]])

    def test_get_current_block(self):
        block_dims = {'block_dims': 1}
        block_dims_1 = {"1-2": [1]}
        ai_core_data = [(1, 2), (1, 2), [(3, 4), (3, 4)]]
        ai_core_data_1 = [1,  2, [(3, 4), (3, 4)]]
        task_id_index = 0
        stream_id_index = 1
        key = UpdateAICoreData(CONFIG)
        result = key._get_current_block(block_dims, ai_core_data, task_id_index, stream_id_index)
        result_1 = key._get_current_block(block_dims_1, ai_core_data_1, task_id_index, stream_id_index)
        self.assertEqual(result, 0)
        self.assertEqual(result_1, 1)

    def test_get_col_indexes_to_cal_total_if(self):
        headers = ["task_id", "stream_id", "total_cycles"]
        key = UpdateAICoreData(CONFIG)
        result = key._UpdateAICoreData__get_col_indexes_to_cal_total(headers)
        self.assertEqual(result, (0, 1, 2))

    def test_get_col_indexes_to_cal_total_error(self):
        headers = [None, None, None]
        with mock.patch(NAMESPACE + '.logging.error'):
            key = UpdateAICoreData(CONFIG)
            result = key._UpdateAICoreData__get_col_indexes_to_cal_total(headers)
            self.assertEqual(result, (-1, -1, -1))

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
