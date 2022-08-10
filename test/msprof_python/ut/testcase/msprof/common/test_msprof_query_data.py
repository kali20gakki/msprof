import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_query_data import MsprofQueryData

from sqlite.db_manager import DBManager
import sqlite3

NAMESPACE = 'common_func.msprof_query_data'


class TestMsprofQueryData(unittest.TestCase):
    def setUp(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_GE_INFO))

    def tearDown(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_GE_INFO))

    def test_get_job_basic_info_1(self):
        InfoConfReader()._info_json = {"DeviceInfo": None, "devices": [1, 2]}
        InfoConfReader()._start_info = {"collectionDateBegin": None}
        InfoConfReader()._end_info = {"collectionDateEnd": None}
        key = MsprofQueryData('123')
        result = key.get_job_basic_info()
        self.assertEqual(result, [])

    def test_get_job_basic_info_2(self):
        InfoConfReader()._info_json = {"DeviceInfo": 123, "devices": [1, 2]}
        InfoConfReader()._start_info = {"collectionDateBegin": None}
        InfoConfReader()._end_info = {"collectionDateEnd": None}
        key = MsprofQueryData('123')
        result = key.get_job_basic_info()
        self.assertEqual(result, [])

    def test_get_job_basic_info_3(self):
        InfoConfReader()._info_json = {"DeviceInfo": None, "devices": None, "jobInfo": None}
        InfoConfReader()._start_info = {"collectionDateBegin": 1}
        InfoConfReader()._end_info = {"collectionDateEnd": 2}
        key = MsprofQueryData('123')
        result = key.get_job_basic_info()
        self.assertEqual(result, [None, 'N/A', '123', 1])

    def test_get_job_iteration_info(self):
        create_sql_ge = "create table IF NOT EXISTS TaskInfo (model_id INT)"
        data_ge = ((1,),)
        insert_sql_ge = "insert into {0} values ({value})".format("TaskInfo",
                                                                  value="?," * (len(data_ge[0]) - 1) + "?")

        create_sql = "create table IF NOT EXISTS step_trace_data (index_id INT,model_id INT,step_start," \
                     "step_end,iter_id INT default 1, ai_core_num INT default 0)"
        data = ((1, 1, 118562263836, 118571743672, 1, 60),)
        insert_sql = "insert into {0} values ({value})".format("step_trace_data",
                                                               value="?," * (len(data[0]) - 1) + "?")

        db_manager = DBManager()
        res_ge = db_manager.create_table("ge_info.db", create_sql_ge, insert_sql_ge, data_ge)
        res = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        res_ge[0].close()
        res[0].close()

        # res = db_manager.create_table("step_trace.db")
        key = MsprofQueryData(db_manager.db_path + "/../")
        result = key.get_job_iteration_info()
        self.assertEqual(result, [[1, 1, '1']])

        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"):
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
                with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                    with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                        key = MsprofQueryData('123')
                        result = key.get_job_iteration_info()
                    self.assertEqual(result, [])

                with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', side_effect=sqlite3.DatabaseError):
                    with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                        key = MsprofQueryData('123')
                        result = key.get_job_iteration_info()
                    self.assertEqual(result, [])

    def test_update_top_iteration_info(self):
        msprof_query_data = MsprofQueryData('123')
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        create_sql = "create table IF NOT EXISTS step_trace_data (index_id INT,model_id INT,step_start," \
                     "step_end,iter_id INT default 1, ai_core_num INT default 0)"
        data = ((1, 1, 118562263836, 118571743672, 1, 60), (2, 1, 118562263866, 118571743612, 1, 60),)
        insert_sql = "insert into {0} values ({value})".format("step_trace_data",
                                                               value="?," * (len(data[0]) - 1) + "?")
        res = _db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        result = msprof_query_data._update_top_iteration_info([], {1}, res[0])
        self.assertEqual(result, [])
        result = msprof_query_data._update_top_iteration_info([(1, 1), ], {1}, res[0])
        self.assertEqual(result, [])
        result = msprof_query_data._update_top_iteration_info([(1, 1), ], {1}, res[1])
        self.assertEqual(result, [[1, 1, "1,2"]])
        result = msprof_query_data._update_top_iteration_info([(1, 1), ], {2}, res[1])
        self.assertEqual(result, [[1, 1, "N/A"]])
        _db_manager.clear_table("step_trace_data")
        result = msprof_query_data._update_top_iteration_info([(1, 1), ], {1}, res[1])
        self.assertEqual(result, [])

    def test_assembly_job_info_1(self):
        basic_data = [1, 2, 3, 4]
        iteration_data = None
        key = MsprofQueryData('123')
        result = key.assembly_job_info(basic_data, iteration_data)
        self.assertEqual(len(result), 1)

    def test_assembly_job_info_2(self):
        basic_data = [1, 2, 3, 4]
        iteration_data = [(1, 1, "1"), (2, 2, "2,1")]
        key = MsprofQueryData('123')
        result = key.assembly_job_info(basic_data, iteration_data)
        self.assertEqual(len(result), 2)

    def test_query_data(self):
        with mock.patch(NAMESPACE + '.MsprofQueryData.get_job_basic_info', return_value=None):
            key = MsprofQueryData('123')
            result = key.query_data()
        self.assertEqual(result, [])
        with mock.patch(NAMESPACE + '.MsprofQueryData.get_job_basic_info', return_value=[1, 2, 3, 4]), \
             mock.patch(NAMESPACE + '.MsprofQueryData.get_job_iteration_info',
                        return_value=[(1, 1, "1"), (2, 2, "2,1")]):
            key = MsprofQueryData('123')
            result = key.query_data()
        self.assertEqual(len(result), 2)


if __name__ == '__main__':
    unittest.main()
