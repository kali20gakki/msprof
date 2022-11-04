import unittest
from collections import OrderedDict
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_iteration import MsprofIteration
from common_func.db_name_constant import DBNameConstant

from constant.constant import INFO_JSON
from sqlite.db_manager import DBManager
import sqlite3

NAMESPACE = 'common_func.msprof_iteration'


class TestMsprofIteration(unittest.TestCase):
    def setUp(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def tearDown(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def test_get_iteration_time(self):
        index_id = 1
        model_id = 1
        with mock.patch(NAMESPACE + '.Utils.is_step_scene', return_value=True):
            key = MsprofIteration('123')
            result = key.get_iteration_time(index_id, model_id)
        self.assertEqual(result, [])

    def test_get_step_iteration_time_1(self):
        index_id = 1
        model_id = 1
        create_sql = "create table IF NOT EXISTS step_trace_data (index_id INT,model_id INT," \
                     "step_start,step_end,iter_id INT default 1, ai_core_num INT default 0)"
        data = ((1, 1, 118562263836, 118571743672, 1, 60),)
        insert_sql = "insert into {0} values ({value})".format("step_trace_data",
                                                               value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=res), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            key = MsprofIteration('123')
            InfoConfReader()._info_json = INFO_JSON
            result = key.get_step_iteration_time(index_id, model_id)
        self.assertEqual(len(result), 2)
        with mock.patch(NAMESPACE + '.MsprofIteration._get_iteration_time', side_effect=sqlite3.Error), \
             mock.patch(NAMESPACE + '.logging.error'):
            key = MsprofIteration('123')
            result = key.get_step_iteration_time(index_id, model_id)
        self.assertEqual(result, [])
        db_manager = DBManager()
        res = db_manager.create_table("step_trace.db")
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            key = MsprofIteration('123')
            key.get_step_iteration_time(index_id, model_id)
            res[1].execute("drop table step_trace_data")
            res[0].commit()
            db_manager.destroy(res)

    def test_get_step_iteration_time_2(self):
        index_id = 1
        model_id = 2
        create_sql = "create table IF NOT EXISTS step_trace_data (index_id INT,model_id INT," \
                     "step_start,step_end,iter_id INT default 1, ai_core_num INT default 0)"
        data = ((1, 2, 118562263836, 118571743672, 1, 60),
                (2, 2, 118571743772, 118571743872, 2, 60))
        insert_sql = "insert into {0} values ({value})".format("step_trace_data",
                                                               value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        InfoConfReader()._info_json = {'pid': 1, "DeviceInfo": [{'hwts_frequency': 1000}]}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=res), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            key = MsprofIteration('123')
            result = key._generate_trace_result(key.get_step_iteration_time(index_id, model_id))
        self.assertEqual(result, [(0.0, 118571743.67199999)])
        db_manager = DBManager()
        res = db_manager.create_table("step_trace.db")
        res[1].execute("drop table step_trace_data")
        res[0].commit()
        db_manager.destroy(res)

    def test_get_train_iteration_time_1(self):
        iteration_id = 1
        create_sql = "create table IF NOT EXISTS training_trace (host_id int, device_id int, iteration_id int, " \
                     "job_stream int, job_task int, FP_start int, FP_stream int, FP_task int, BP_end int, " \
                     "BP_stream int, BP_task int, iteration_end int, iter_stream int,iter_task int, " \
                     "iteration_time int, fp_bp_time int, grad_refresh_bound int, data_aug_bound int default 0)"
        data = ((0, 4, 1, 58, 140, 1695384646479, 58, 141, 1695386378943, 60, 205, 1695387952406,
                 57, 5, 3305927, 1732464, 1573463, 0),)
        insert_sql = "insert into {0} values ({value})".format("training_trace",
                                                               value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("trace.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="trace.db"), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=res):
            key = MsprofIteration('123')
            InfoConfReader()._info_json = INFO_JSON
            result = key._generate_trace_result(key.get_train_iteration_time(iteration_id))
        self.assertEqual(result, [(0.0, 44150727927.239586)])
        db_manager = DBManager()
        res = db_manager.create_table("trace.db")
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            key = MsprofIteration('123')
            key.get_train_iteration_time(iteration_id)
        res[1].execute("drop table training_trace")
        res[0].commit()
        db_manager.destroy(res)

    def test_get_train_iteration_time_2(self):
        iteration_id = 902
        create_sql = "create table IF NOT EXISTS training_trace (host_id int,device_id int, iteration_id int, " \
                     "job_stream int, job_task int, FP_start int, FP_stream int, FP_task int, BP_end int, " \
                     "BP_stream int, BP_task int, iteration_end int, iter_stream int,iter_task int, " \
                     "iteration_time int, fp_bp_time int, grad_refresh_bound int, data_aug_bound int default 0)"
        trace_data = ((0, 4, 1, 58, 140, 1695384646478, 58, 141, 1695386378943, 60, 205, 1695387952406,
                       57, 5, 3305927, 1732464, 1573463, 0),)
        insert_sql = "insert into {0} values ({value})".format("training_trace",
                                                               value="?," * (len(trace_data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("trace.db", create_sql, insert_sql, trace_data)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="trace.db"), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=res):
            key = MsprofIteration('123')
            InfoConfReader()._info_json = INFO_JSON
            result = key.get_train_iteration_time(iteration_id)
        self.assertEqual(result, [])
        db_manager = DBManager()
        res = db_manager.create_table("trace.db")
        res[1].execute("drop table training_trace")
        res[0].commit()
        db_manager.destroy(res)

    def test_get_trace_iteration_end(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
             mock.patch(NAMESPACE + '.MsprofIteration._generate_trace_iter_end_result', return_value=1), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=1), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"):
            key = MsprofIteration('123')
            result = key._MsprofIteration__get_trace_iteration_end()
        self.assertEqual(result, 1)

        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False), \
             mock.patch(NAMESPACE + '.MsprofIteration._generate_trace_iter_end_result', return_value=1), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=1), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"):
            key = MsprofIteration('123')
            result = key._MsprofIteration__get_trace_iteration_end()
        self.assertEqual(result, OrderedDict())

        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
             mock.patch(NAMESPACE + '.MsprofIteration._generate_trace_iter_end_result', return_value=1), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"):
            key = MsprofIteration('123')
            result = key._MsprofIteration__get_trace_iteration_end()
        self.assertEqual(result, OrderedDict())


if __name__ == '__main__':
    unittest.main()
