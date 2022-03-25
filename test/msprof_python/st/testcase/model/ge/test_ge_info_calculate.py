import unittest
import os
from unittest import mock
from sqlite.db_manager import DBManager
from model.ge.ge_info_calculate_model import GeInfoModel
from common_func.constant import Constant

NAMESPACE = 'model.ge.ge_info_calculate_model'


class TestGeInfoModel(unittest.TestCase):

    def test_check_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('ge_info.db')
        with mock.patch(NAMESPACE + '.logging.warning'), \
             mock.patch(NAMESPACE + '.PathManager.get_data_dir'), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = GeInfoModel('test')
            result = check.check_table()
            self.assertFalse(result)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = GeInfoModel('test')
            check.conn, check.cur = res[0], res[1]
            result = check.check_table()
            self.assertTrue(result)
        db_manager.destroy(res)

    def test_map_model_to_iter(self):
        db_name = "step_trace.db"
        table_name = "step_trace_data"
        db_manager = DBManager()
        if os.path.exists(os.path.join(db_manager.db_path, db_name)):
            os.remove(os.path.join(db_manager.db_path, db_name))

        create_sql = "CREATE TABLE IF NOT EXISTS {0} (model_id int, index_id int, iter_id int)".format(table_name)

        data = ((1, 1, 1),)
        insert_sql = "insert into {0} values ({1})".format(table_name, "?," * (len(data[0]) - 1) + "?")

        conn, curs = db_manager.create_table(db_name, create_sql, insert_sql, data)

        check = GeInfoModel(os.path.join(db_manager.db_path, "../"))

        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
             mock.patch(NAMESPACE + ".DBManager.create_connect_db", return_value=(conn, curs)):
            res = check.map_model_to_iter()

        if os.path.exists(db_manager.db_name):
            os.remove(db_manager.db_name)
        self.assertEqual(res, {(1, 1): 1})

    def test_get_ge_data_step(self):
        with mock.patch(NAMESPACE + '.Utils.is_step_scene', return_value=True), \
             mock.patch(NAMESPACE + '.GeInfoModel._GeInfoModel__get_ge_data_step_scene', return_value=[]):
            check = GeInfoModel("")
            result = check.get_ge_data(Constant.TASK_TYPE_AI_CORE)
        self.assertEqual(result, {})

    def test_get_ge_data_train(self):
        with mock.patch(NAMESPACE + '.Utils.is_step_scene', return_value=False), \
             mock.patch(NAMESPACE + '.Utils.is_training_trace_scene', return_value=True), \
             mock.patch(NAMESPACE + '.GeInfoModel._GeInfoModel__get_ge_data_training_trace_scene', return_value=[]):
            check = GeInfoModel("")
            result = check.get_ge_data(Constant.TASK_TYPE_AI_CORE)
        self.assertEqual(result, {})

    def test_get_ge_data_step_scene(self):
        with mock.patch(NAMESPACE + '.GeInfoModel.map_model_to_iter', return_value=[]), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.GeInfoModel._GeInfoModel__update_iter_dict', return_value=[]):
            check = GeInfoModel("")
            check._GeInfoModel__get_ge_data_step_scene({}, Constant.TASK_TYPE_AI_CORE)

        model_to_iter_dict = {(4294967295, 1): 1, (4294967295, 2): 2}
        ge_dynamic_data = [(4294967295, 1, 2, 9465, 0)]
        with mock.patch(NAMESPACE + '.GeInfoModel.map_model_to_iter', return_value=model_to_iter_dict), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=ge_dynamic_data), \
             mock.patch(NAMESPACE + '.GeInfoModel._GeInfoModel__update_iter_dict', return_value=[]):
            check = GeInfoModel("")
            ge_dict = {}
            check._GeInfoModel__get_ge_data_step_scene(ge_dict, Constant.TASK_TYPE_AI_CORE)
            self.assertEqual(ge_dict, {'1': {'2-9465-0'}})

    def test_get_batch_dict1(self):
        expect_res = {(-1, 2): (3, 4)}
        check = GeInfoModel("")
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[(500000, 90000000, 2, 3, 4)]), \
                mock.patch(NAMESPACE + ".Utils.is_single_op_scene", return_value=True):
            res = check.get_batch_dict("AI_CORE")
        self.assertEqual(expect_res, res)

    def test_get_batch_dict2(self):
        expect_res = {(5, 2): (3, 4)}
        check = GeInfoModel("")
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[(10, 3, 2, 3, 4)]), \
                mock.patch(NAMESPACE + ".Utils.is_single_op_scene", return_value=False), \
                mock.patch(NAMESPACE + ".GeInfoModel.map_model_to_iter", return_value={(10, 3): 5}):
            res = check.get_batch_dict("AI_CORE")
        self.assertEqual(expect_res, res)
