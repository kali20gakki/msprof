import unittest
from unittest import mock

from mscalculate.data_analysis.op_summary_op_scene_calculator import OpSummaryOpSceneCalculator
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from sqlite.db_manager import DBManager

file_list = {}
NAMESPACE = 'mscalculate.data_analysis.op_summary_op_scene_calculator'


class TestOpSummaryOpSceneCalculator(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator.process'):
            key = OpSummaryOpSceneCalculator(file_list, CONFIG)
            key.ms_run()

    def test_process(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test.text'), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator.create_summary_table'):
            check = OpSummaryOpSceneCalculator(file_list, CONFIG)
            check.process()

    def test_get_ge_sql(self):
        check = OpSummaryOpSceneCalculator(file_list, CONFIG)
        result = check._get_ge_sql()
        sql = "SELECT model_id, task_id, stream_id, op_name, op_type, block_dim, mix_block_dim, " \
              "task_type, timestamp, batch_id, context_id from TaskInfo"
        self.assertEqual(sql, result)

    def test_create_ge_tensor_table(self):
        with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator._check_tensor_table', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = OpSummaryOpSceneCalculator(file_list, CONFIG)
            result = check.create_ge_tensor_table()
        self.assertFalse(result)
        with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator._check_tensor_table', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                mock.patch(NAMESPACE + '.DBManager.executemany_sql'), \
                mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator._get_tensor_data', return_value=[[1]]):
            check = OpSummaryOpSceneCalculator(file_list, CONFIG)
            result = check.create_ge_tensor_table()
        self.assertTrue(result)

    def test_get_ge_merge_data(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(False, False)), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = OpSummaryOpSceneCalculator(file_list, CONFIG)
            _get_ge_merge_data = getattr(check, "_get_ge_merge_data")
            result = _get_ge_merge_data()
        self.assertEqual(result, [])

        expect_res = [(1, 3, 5, 4, 'global_step/Assign', 'Assign', 2, 'AI_CORE', 176988158851790)]
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=expect_res):
            check = OpSummaryOpSceneCalculator(file_list, CONFIG)
            _get_ge_merge_data = getattr(check, "_get_ge_merge_data")
            result = _get_ge_merge_data()
        self.assertEqual(result, expect_res)

    def test_create_core_table(self):
        curs, table_name = None, 'test'
        cols_with_type = {'total_time': 'numeric', 'total_cycles': 'numeric'}
        with mock.patch(NAMESPACE + '.DBManager.get_table_info', return_value=cols_with_type):
            check = OpSummaryOpSceneCalculator(file_list, CONFIG)
            result = check._create_core_table(curs, table_name)
        self.assertEqual(result, "[total_time] numeric,[total_cycles] numeric")

    def test_get_ai_core_metric(self):
        table_name = 'MetricSummary'
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(False, False)), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = OpSummaryOpSceneCalculator(file_list, CONFIG)
            _get_ai_core_metric = getattr(check, "_get_ai_core_metric")
            result = _get_ai_core_metric(table_name)
        self.assertEqual(result, [])
        create_sql = "CREATE TABLE IF NOT EXISTS MetricSummary (total_time numeric,total_cycles numeric," \
                     "mac_fp16_ratio numeric,mac_int8_ratio numeric,vec_fp32_ratio numeric,vec_fp16_ratio numeric," \
                     "vec_int32_ratio numeric,vec_misc_ratio numeric,cube_fops numeric,vector_fops numeric," \
                     "device_id INT,task_id INT,stream_id INT,index_id INT,model_id INT)"
        insert_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)".format('MetricSummary')
        data = ((1, 1, 2, 3, 4, 5, 1, 1, 2, 3, 4, 5, 3, 4, 5),)
        db_manager = DBManager()
        res = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=res), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = OpSummaryOpSceneCalculator(file_list, CONFIG)
            check.conn, check.curs = res
            _get_ai_core_metric = getattr(check, "_get_ai_core_metric")
            result = _get_ai_core_metric(table_name)
        self.assertEqual(result, [(1, 1, 2, 3, 4, 5, 1, 1, 2, 3, 4, 5, 3, 4, 5)])
        res[1].execute("drop table MetricSummary")
        res[0].commit()
        db_manager.destroy(res)

    def test_create_ge_summary_table(self):
        ge_merge_data = ((1, 2, 3),)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test\\test.text'):
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                result = check.create_ge_summary_table()
            self.assertEqual(result, False)
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
                with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator._get_ge_merge_data', return_value=False):
                    check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                    result = check.create_ge_summary_table()
                self.assertEqual(result, False)
                with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator._get_ge_merge_data',
                                return_value=ge_merge_data), \
                        mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=0), \
                        mock.patch(NAMESPACE + '.DBManager.execute_sql', return_value=0), \
                        mock.patch(NAMESPACE + '.DBManager.executemany_sql', return_value=0):
                    check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                    result = check.create_ge_summary_table()
                self.assertEqual(result, True)

    def test_create_ai_core_metrics_table(self):
        core_merge_data = ((1, 2, 3),)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test\\test.text'):
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
                with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator._get_ai_core_metric', return_value=None):
                    check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                    result = check.create_ai_core_metrics_table()
                self.assertEqual(result, False)
                with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator._get_ai_core_metric',
                                return_value=core_merge_data), \
                        mock.patch(NAMESPACE + '.DBManager.executemany_sql'):
                    check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                    result = check.create_ai_core_metrics_table()
                self.assertEqual(result, True)
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                result = check.create_ai_core_metrics_table()
            self.assertEqual(result, False)

    def test_get_task_time_data(self):
        InfoConfReader()._info_json = {'devices': ['0']}
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_task_time_data',
                        return_value=True):
            with mock.patch(NAMESPACE + '.GetOpTableTsTime.get_task_time_data', return_value=[]), \
                    mock.patch(NAMESPACE + '.OpCommonFunc.calculate_task_time',
                               return_value=((1, 2, 3),)):
                check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                result = check.get_task_time_data()
            self.assertEqual(result, ((1, 2, 3),))
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_task_time_data',
                        return_value=False):
            check = OpSummaryOpSceneCalculator(file_list, CONFIG)
            result = check.get_task_time_data()
        self.assertEqual(result, [])

    def test_create_task_time_table(self):
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator.get_task_time_data', return_value=False):
                check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                result = check.create_task_time_table()
            self.assertEqual(result, False)
            with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator.get_task_time_data',
                            return_value=[(1, 2, 3, 4, 5, 6, 7, 8, 9)]), \
                    mock.patch(NAMESPACE + '.OpCommonFunc.deal_batch_id', return_value=((1, 2, 3),)), \
                    mock.patch(NAMESPACE + '.DBManager.executemany_sql'):
                check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                result = check.create_task_time_table()
            self.assertTrue(result)

    def test_create_summary_table(self):
        create_sql = "create table if not exists ge_task_data (device_id INTEGER," \
                     "model_name TEXT,model_id INTEGER,op_name TEXT,stream_id INTEGER," \
                     "task_id INTEGER,block_dim INTEGER,op_state TEXT,task_type TEXT," \
                     "op_type TEXT,iter_id INTEGER,input_count INTEGER,input_formats TEXT," \
                     "input_data_types TEXT,input_shapes TEXT,output_count INTEGER," \
                     "output_formats TEXT,output_data_types TEXT,output_shapes TEXT)"
        insert_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)".format('ge_task_data')
        data = ((0, 'test', 1, 'model', 5, 3, 2, 'static', 'AI_CORE', 'trans_data',
                 1, 1, '12', '1750', '1752', 0, 'test', 'test2', 'test3'),)
        db_manager = DBManager()
        res = db_manager.create_table("ge_info.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test\\test.text'):
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False), \
                    mock.patch(NAMESPACE + '.logging.warning'), \
                    mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"):
                check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                check.create_summary_table()

            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=res):
                with mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator.create_ge_summary_table',
                                return_value=False), \
                        mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator.create_ai_core_metrics_table',
                                   return_value=False), \
                        mock.patch(NAMESPACE + '.OpSummaryOpSceneCalculator.create_task_time_table',
                                   return_value=False), \
                        mock.patch(NAMESPACE + '.logging.warning'), \
                        mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"):
                    check = OpSummaryOpSceneCalculator(file_list, CONFIG)
                    check.create_summary_table()
        res[1].execute('drop table ge_task_data')
        db_manager.destroy(res)


if __name__ == '__main__':
    unittest.main()
