import unittest
from unittest import mock

from constant.ut_db_name_constant import DB_TRACE
from constant.ut_db_name_constant import TABLE_ALL_REDUCE
from constant.ut_db_name_constant import TABLE_TRAINING_TRACE
from saver.training.training_trace_saver import TrainingTraceSaver
from common_func.db_name_constant import DBNameConstant
from sqlite.db_manager import DBManager
import sqlite3

from sqlite.db_manager import DBOpen

NAMESPACE = 'saver.training.training_trace_saver'


class TestTrainingTraceSaver(unittest.TestCase):
    def setUp(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def tearDown(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def test_save_data_to_db_1(self):
        message = {"data": ({"iteration_id": 1, "job_stream": 2, "job_task": 3, "FP_start": 4,
                             "FP_stream": 5, "FP_task": 11, "BP_end": 22, "BP_stream": 33,
                             "BP_task": 44, "iteration_end": 55, "iter_stream": 66, "iter_task": 77,
                             "all_reduces": ({"start": 111, "start_stream": 222, "start_task": 333, "end": 444,
                                              "end_stream": 555, "end_task": 666},), },), "host_id": 123,
                   "device_id": 456, "sql_path": 123}
        with mock.patch(NAMESPACE + '.check_number_valid', return_value=False), \
                mock.patch(NAMESPACE + '.logging.error'):
            key = TrainingTraceSaver()
            result = key.save_data_to_db(message)
        self.assertEqual(result, False)
        with DBOpen("time.db") as db_open:
            with mock.patch(NAMESPACE + '.check_number_valid', return_value=True), \
                 mock.patch(NAMESPACE + '.TrainingTraceSaver.create_trace_conn', return_value=(db_open.db_conn, db_open.db_curs)), \
                 mock.patch(NAMESPACE + '.TrainingTraceSaver._TrainingTraceSaver__insert_values'), \
                 mock.patch(NAMESPACE + '.TrainingTraceSaver._TrainingTraceSaver__update_iteration_id'), \
                 mock.patch(NAMESPACE + '.TrainingTraceSaver._TrainingTraceSaver__update_data_aug_bound'):
                key = TrainingTraceSaver()
                result = key.save_data_to_db(message)
        self.assertEqual(result, True)

    def test_save_data_to_db_2(self):
        message = None
        with mock.patch(NAMESPACE + '.logging.error'):
            key = TrainingTraceSaver()
            result = key.save_data_to_db(message)
        self.assertEqual(result, False)

    def test_save_file_relation_to_db_1(self):
        message = {"sql_path": (123,), "file_path": 'home\\sql'}
        create_sql = "create table IF NOT EXISTS files (file_path text)"
        data = (('home',),)

        with DBOpen("trace.db") as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data("files", data)
            with mock.patch(NAMESPACE + '.TrainingTraceSaver.create_trace_conn',
                            return_value=(db_open.db_conn, db_open.db_curs)):
                key = TrainingTraceSaver()
                key.save_file_relation_to_db(message)

    def test_save_file_relation_to_db_2(self):
        message = None
        with mock.patch(NAMESPACE + '.logging.error'):
            key = TrainingTraceSaver()
            result = key.save_file_relation_to_db(message)
        self.assertEqual(result, False)

    def test_create_trace_conn(self):
        sql_path = 'home\\sql'
        create_sql = "create table IF NOT EXISTS training_trace (host_id int,device_id int, iteration_id int, " \
                     "job_stream int, job_task int, FP_start int, FP_stream int, FP_task int, BP_end int, " \
                     "BP_stream int, BP_task int, iteration_end int, iter_stream int,iter_task int, " \
                     "iteration_time int, fp_bp_time int, grad_refresh_bound int, data_aug_bound int default 0)"
        data = ((0, 4, 1, 58, 140, 1695384646479, 58, 142, 1695386378943, 60, 205, 1695387952406,
                 57, 5, 3305927, 1732464, 1573463, 0),)
        create_sql_1 = "create table IF NOT EXISTS all_reduce (host_id int,device_id int,iteration_end int, " \
                       "start int, start_stream int, start_task int,end int, end_stream int, end_task int" \
                       ")"
        data_1 = ((0, 4, 1695387952406, 1695385794913, 59, 4, 1695386978301, 59, 103),)

        with DBOpen(DB_TRACE) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(TABLE_TRAINING_TRACE, data)
            db_open.create_table(create_sql_1)
            db_open.insert_data(TABLE_ALL_REDUCE, data_1)
            with mock.patch('os.path.join', return_value=DB_TRACE):
                with mock.patch('os.path.exists', return_value=True), \
                        mock.patch('common_func.db_manager.DBManager.create_connect_db',
                                   return_value=(db_open.db_conn, db_open.db_curs)):
                    key = TrainingTraceSaver()
                    key.create_trace_conn(sql_path)
                with mock.patch('os.path.exists', return_value=False), \
                     mock.patch('common_func.db_manager.DBManager.create_connect_db',
                                return_value=(db_open.db_conn, db_open.db_curs)), \
                     mock.patch('os.chmod'):
                    key = TrainingTraceSaver()
                    result = key.create_trace_conn(sql_path)
                self.assertEqual(2, len(result))

    def test_insert_values(self):
        host_id = 123
        device_id = 456
        data = ({"FP_start": 1, "job_stream": 2, "job_task": 3, "FP_stream": 4, "FP_task": 5, "BP_end": 6,
                 "BP_stream": 7, "BP_task": 8, "iter_stream": 9, "iter_task": 8, "iteration_end": 7,
                 "iteration_id": 1,
                 "all_reduces": ({"start": 1, "start_stream": 2, "start_task": 3, "end": 4,
                                  "end_stream": 5, "end_task": 6},)},)
        create_sql = "create table IF NOT EXISTS training_trace (host_id int,device_id int, iteration_id int, " \
                     "job_stream int, job_task int, FP_start int, FP_stream int, FP_task int, BP_end int, " \
                     "BP_stream int, BP_task int, iteration_end int, iter_stream int,iter_task int, " \
                     "iteration_time int, fp_bp_time int, grad_refresh_bound int, data_aug_bound int default 0)"
        data_0 = ((0, 4, 1, 58, 140, 1695384646479, 58, 141, 1695386378944, 60, 205, 1695387952406,
                   57, 5, 3305927, 1732464, 1573463, 0),)
        insert_sql = "insert into {0} values ({value})".format("training_trace",
                                                               value="?," * (len(data_0[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("trace.db", create_sql, insert_sql, data_0)
        create_sql_1 = "create table IF NOT EXISTS all_reduce (host_id int, device_id int,iteration_end int, " \
                       "start int, start_stream int, start_task int,end int, end_stream int, end_task int" \
                       ")"
        data_1 = ((0, 4, 1695387952406, 1695385794913, 59, 4, 1695386978300, 59, 103),)
        insert_sql_1 = "insert into {0} values ({value})".format("all_reduce",
                                                                 value="?," * (len(data_1[0]) - 1) + "?")
        db_manager.destroy(res)
        res = db_manager.create_table("trace.db", create_sql_1, insert_sql_1, data_1)
        conn = res[0]
        key = TrainingTraceSaver()
        key._TrainingTraceSaver__insert_values(conn, host_id, device_id, data)
        res[1].execute("drop table all_reduce")
        res[0].commit()
        res[0].commit()
        db_manager.destroy(res)

    def test_update_iteration_id(self):
        create_sql = "create table IF NOT EXISTS {} (host_id int, device_id int, iteration_id int, " \
                     "job_stream int, job_task int, FP_start int, FP_stream int, FP_task int, BP_end int, " \
                     "BP_stream int, BP_task int, iteration_end int, iter_stream int,iter_task int, " \
                     "iteration_time int, fp_bp_time int,grad_refresh_bound int, " \
                     "data_aug_bound int default 0)".format(DBNameConstant.TABLE_TRAINING_TRACE)
        data_0 = ((0, 4, 1, 58, 140, 1695384646479, 58, 141, 1695386378947, 60, 205, 1695387952406,
                   57, 5, 3305927, 1732464, 1573463, 0),)
        insert_sql = "insert into {0} values ({value})".format(DBNameConstant.TABLE_TRAINING_TRACE,
                                                               value="?," * (len(data_0[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("trace.db", create_sql, insert_sql, data_0)
        conn = res[0]
        key = TrainingTraceSaver()
        key._TrainingTraceSaver__update_iteration_id(conn)
        res[1].execute("drop table {}".format(DBNameConstant.TABLE_TRAINING_TRACE))
        res[0].commit()
        db_manager.destroy(res)

    def test_update_data_aug_bound(self):
        device_id = 123
        create_sql = "create table IF NOT EXISTS {} (host_id int, device_id int, iteration_id int, " \
                     "job_stream int, job_task int, FP_start int, FP_stream int, FP_task int, BP_end int, " \
                     "BP_stream int, BP_task int, iteration_end int, iter_stream int,iter_task int, " \
                     "iteration_time int, fp_bp_time int, grad_refresh_bound int, " \
                     "data_aug_bound int default 0)".format(DBNameConstant.TABLE_TRAINING_TRACE)
        data_0 = ((0, 4, 1, 58, 140, 1695384646479, 58, 141, 1695386378943, 60, 205, 1695387952406,
                   57, 5, 3305927, 1732464, 1573463, 0),)
        insert_sql = "insert into {0} values ({value})".format(DBNameConstant.TABLE_TRAINING_TRACE,
                                                               value="?," * (len(data_0[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("trace.db", create_sql, insert_sql, data_0)
        conn = res[0]
        key = TrainingTraceSaver()
        key._TrainingTraceSaver__update_data_aug_bound(conn, device_id)
        res[1].execute("drop table {}".format(DBNameConstant.TABLE_TRAINING_TRACE))
        res[0].commit()
        db_manager.destroy(res)


if __name__ == '__main__':
    unittest.main()
