import sqlite3
import unittest
from unittest import mock

import pytest

from analyzer.create_ai_core_op_summary_db import ParseAiCoreOpSummary
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException

from constant.constant import CONFIG
from sqlite.db_manager import DBManager

NAMESPACE = 'analyzer.create_ai_core_op_summary_db'


class TestParseAiCoreOpSummary(unittest.TestCase):
    def test_init_params(self):
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch('os.path.exists', return_value=False), \
             mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=True), \
             pytest.raises(ProfException) as err:
            check = ParseAiCoreOpSummary(CONFIG)
            check.init_params()
            self.assertEqual(err.value.code, 10)
            with mock.patch('os.path.exists', return_value=True), \
                 mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=True), \
                 mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True), \
                 mock.patch('os.remove'):
                check = ParseAiCoreOpSummary(CONFIG)
                check.init_params()

    def test_create_summary_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('ge_info.db')
        with mock.patch(NAMESPACE + '.logging.warning'), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.get_db_path', return_value=True):

            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                check = ParseAiCoreOpSummary(CONFIG)
                result = check.create_summary_table()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                 mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.create_conn', return_value=res), \
                 mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.create_ge_summary_table',
                            return_value=False), \
                 mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.create_ai_core_metrics_table',
                            return_value=False), \
                 mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.create_task_time_table',
                            return_value=False):
                check = ParseAiCoreOpSummary(CONFIG)
                check.create_summary_table()
        db_manager.destroy(res)

    def test_create_conn(self):
        db_manager = DBManager()
        res = db_manager.create_table('ai_core_op_summary.db')
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.get_db_path', return_value='test\\test'), \
             mock.patch('sqlite3.connect', return_value=res[0]), \
             mock.patch('os.chmod'):
            check = ParseAiCoreOpSummary(CONFIG)
            check.create_conn()
        db_manager.destroy(res)

    def test_get_ge_sql(self):
        ge_sql = "SELECT model_id, batch_id, task_id, stream_id, op_name, op_type, block_dim, task_type, timestamp from TaskInfo where " \
                 "(index_id=? or index_id=0)"
        ge_step_sql = "SELECT model_id, batch_id, task_id, stream_id, op_name, op_type, block_dim, task_type, timestamp from TaskInfo" \
                      " where (index_id=? or index_id=0) and model_id=?"
        with mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                        return_value="step_info"):
            check = ParseAiCoreOpSummary(CONFIG)
            ProfilingScene().init('')
            result = check._get_ge_sql()
        self.assertEqual(result[0], ge_step_sql)
        self.assertEqual(result[1], (1, -1))
        with mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                        return_value="train"):
            check = ParseAiCoreOpSummary(CONFIG)
            ProfilingScene().init('')
            result = check._get_ge_sql()
        self.assertEqual(result[0], ge_sql)
        self.assertEqual(result[1], (1,))

    def test_create_ge_summary_table(self):
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.get_db_path', return_value='test\\test'), \
             mock.patch(NAMESPACE + '.logging.warning'), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
            check = ParseAiCoreOpSummary(CONFIG)
            check.create_ge_summary_table("")
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.get_db_path', return_value='test\\test'), \
             mock.patch(NAMESPACE + '.logging.warning'), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=False):
            check = ParseAiCoreOpSummary(CONFIG)
            check.create_ge_summary_table("")
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.get_db_path', return_value='test\\test'), \
             mock.patch(NAMESPACE + '.logging.warning'), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
             mock.patch(NAMESPACE + '.ParseAiCoreOpSummary._get_ge_sql', return_value=("", ())), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True):
            check = ParseAiCoreOpSummary(CONFIG)
            check.create_ge_summary_table("")


    def test_create_ai_core_metrics_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('ai_core_op_summary.db')
        create_sql = "CREATE TABLE IF NOT EXISTS MetricSummary (index_id INT,model_id INT,step_start REAL," \
                     " step_end INT,iter_id INT,ai_core_num INT)"
        insert_sql = "insert into {} values (?,?,?,?,?,?)".format('MetricSummary')
        data = ((1, 1, 2, 3, 4, 5),)
        db_manager_rt = DBManager()
        res_runtime = db_manager_rt.create_table("runtime.db", create_sql, insert_sql, data)
        res_runtime[1].execute("create table IF NOT EXISTS AivMetricSummary(index_id INT,model_id INT,"
                               "step_start REAL, step_end INT,iter_id INT,ai_core_num INT)")
        curs = res[1]
        db_manager.attach(res[1], 'runtime.db', 'runtime')
        with mock.patch('os.path.splitext', return_value=('runtime', 'db')):
            with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=False):
                check = ParseAiCoreOpSummary(CONFIG)
                check.create_ai_core_metrics_table(curs)
            with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
                 mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.get_db_path', return_value='test\\test'), \
                 mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
                check = ParseAiCoreOpSummary(CONFIG)
                check.create_ai_core_metrics_table(curs)
            with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
                 mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                check = ParseAiCoreOpSummary(CONFIG)
                check.create_ai_core_metrics_table(curs)
        db_manager.destroy(res)
        db_manager_rt.destroy(res_runtime)

    def test_create_task_time_table(self):
        with mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                        return_value="train"), \
             mock.patch(NAMESPACE + '.logging.warning'),\
             mock.patch(NAMESPACE + '.logging.error'):
            with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=False):
                check = ParseAiCoreOpSummary(CONFIG)
                ProfilingScene().init('')
                ProfilingScene()._scene = "single_op"
                check.create_task_time_table("")
            with mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                 mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.get_task_time_data', return_value=[]), \
                 mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=""):
                check = ParseAiCoreOpSummary(CONFIG)
                ProfilingScene().init('')
                ProfilingScene()._scene = "single_op"
                check.create_task_time_table("")
            with mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                 mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.get_task_time_data', return_value=[[0, 1]]), \
                 mock.patch(NAMESPACE + '.DBManager.executemany_sql'), \
                 mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=""):
                check = ParseAiCoreOpSummary(CONFIG)
                ProfilingScene().init('')
                ProfilingScene()._scene = "single_op"
                check.create_task_time_table("")


    def test_get_task_time_data(self):
        InfoConfReader()._info_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_task_time_data',
                        return_value=True):
            with mock.patch(NAMESPACE + '.GetOpTableTsTime.get_task_time_data',
                            return_value=1), \
                 mock.patch(NAMESPACE + '.OpCommonFunc.calculate_task_time',
                            return_value=1):
                check = ParseAiCoreOpSummary(CONFIG)
                ProfilingScene().init('')
                result = check.get_task_time_data()
            self.assertEqual(result, 1)
            with mock.patch(NAMESPACE + '.GetOpTableTsTime.get_task_time_data',
                            return_value=1), \
                 mock.patch(NAMESPACE + '.OpCommonFunc.calculate_task_time',
                            return_value=1):
                check = ParseAiCoreOpSummary(CONFIG)
                ProfilingScene().init('')
                result = check.get_task_time_data()
            self.assertEqual(result, 1)
            with mock.patch(NAMESPACE + '.GetOpTableTsTime.get_task_time_data',
                            return_value=1), \
                 mock.patch(NAMESPACE + '.OpCommonFunc.calculate_task_time',
                            return_value=1):
                check = ParseAiCoreOpSummary(CONFIG)
                ProfilingScene().init('')
                result = check.get_task_time_data()
            self.assertEqual(result, 1)
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_task_time_data',
                        return_value=False):
            check = ParseAiCoreOpSummary(CONFIG)
            result = check.get_task_time_data()
        self.assertEqual(result, [])

    def test_get_db_path(self):
        db_name = 'test'
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=1):
            check = ParseAiCoreOpSummary(CONFIG)
            result = check.get_db_path(db_name)
        self.assertEqual(result, 1)

    def test_run(self):
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.init_params'), \
             mock.patch(NAMESPACE + '.ParseAiCoreOpSummary.create_summary_table'):
            check = ParseAiCoreOpSummary(CONFIG)
            check.run()
