import os
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_common import MsvpCommonConst
from constant.constant import clear_dt_project
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from mscalculate.memory_copy.memcpy_calculator import MemcpyCalculator

NAMESAPCE = "common_func.msprof_iteration"


def prepar_step_trace_db(db_path):
    conn, curs = DBManager.create_connect_db(db_path)
    columns = ["index_id", "model_id", "step_start", "step_end", "iter_id", "ai_core_num"]
    sql = "create table if not exists {0} ({1})".format(DBNameConstant.TABLE_STEP_TRACE_DATA,
                                                        ",".join(columns))
    conn.execute(sql)

    prepar_step_trace_data(conn, curs)
    prepar_ts_memcpy_data(conn, curs)

    DBManager.destroy_db_connect(conn, curs)


def prepar_step_trace_data(conn, curs):
    columns = ["index_id", "model_id", "step_start", "step_end", "iter_id", "ai_core_num"]
    sql = "create table if not exists {0} ({1})".format(DBNameConstant.TABLE_STEP_TRACE_DATA,
                                                        ",".join(columns))
    conn.execute(sql)

    step_trace_data = [(1, 1, 1000, 2000, 1, 30),
                       (1, 2, 3000, 4000, 2, 40)]

    if conn and step_trace_data:
        sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_STEP_TRACE_DATA, "?," * (len(step_trace_data[0]) - 1) + "?")
        DBManager.executemany_sql(conn, sql, step_trace_data)


def prepar_ts_memcpy_data(conn, curs):
    sql = DBManager.sql_create_general_table(DBNameConstant.TABLE_TS_MEMCPY + "Map", DBNameConstant.TABLE_TS_MEMCPY,
                                             os.path.join(MsvpCommonConst.CONFIG_PATH, 'Tables.ini'))
    curs.execute(sql)
    ts_memcpy_data = [(500, 1, 1, 0), (600, 1, 1, 1), (700, 1, 1, 2), (800, 1, 1, 3), (800, 1, 1, 1),
                      (2500, 1, 2, 0), (2600, 1, 2, 1), (2700, 1, 2, 2), (2800, 2, 1, 0)]

    if conn and ts_memcpy_data:
        sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_TS_MEMCPY, "?," * (len(ts_memcpy_data[0]) - 1) + "?")
        DBManager.executemany_sql(conn, sql, ts_memcpy_data)


class TestMemcpyModel(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_MEMCPY_TEST')

    def setUp(self):
        os.makedirs(os.path.join(self.DIR_PATH, "sqlite"))
        db_path = os.path.join(self.DIR_PATH, "sqlite", DBNameConstant.DB_STEP_TRACE)
        prepar_step_trace_db(db_path)

    def tearDown(self):
        clear_dt_project(self.DIR_PATH)

    def test_calculate_1(self):
        # stream_id, task_id, receive_time, start_time, end_time, duration, name, type
        expect_res = [(1, 2, 2.5, 2.6, 2.7, 0.10000000000000009, 'MemcopyAsync', 'other')]
        memcpy_calculator = MemcpyCalculator({}, {"result_dir": self.DIR_PATH, "iter_id": 1, "model_id": 2})

        with mock.patch('common_func.utils.Utils.get_scene', return_value=Constant.STEP_INFO):
            ProfilingScene().init("")
            InfoJsonReaderManager(InfoJson(pid=1, DeviceInfo=[DeviceInfo(hwts_frequency=1000)])).process()
            memcpy_calculator.calculator_connect_db()
            memcpy_calculator.calculate()
            self.assertEqual(expect_res, memcpy_calculator._memcpy_data)

    def test_calculate_2(self):
        # stream_id, task_id, receive_time, start_time, end_time, duration, name, type
        expect_res = [(1, 1, 0.5, 0.6, 0.7, 0.09999999999999998, 'MemcopyAsync', 'other'),
                      (1, 2, 2.5, 2.6, 2.7, 0.10000000000000009, 'MemcopyAsync', 'other')]
        memcpy_calculator = MemcpyCalculator({}, {"result_dir": self.DIR_PATH, "iter_id": 1, "model_id": 2})

        with mock.patch('common_func.utils.Utils.get_scene', return_value=Constant.SINGLE_OP):
            ProfilingScene().init("")
            InfoJsonReaderManager(InfoJson(pid=1, DeviceInfo=[DeviceInfo(hwts_frequency=1000)])).process()
            memcpy_calculator.calculator_connect_db()
            memcpy_calculator.calculate()
            self.assertEqual(expect_res, memcpy_calculator._memcpy_data)
