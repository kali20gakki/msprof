import os
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from sqlite.db_manager import DBManager
from viewer.training.step_trace_viewer import StepTraceViewer
from constant.info_json_construct import InfoJsonReaderManager
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson

NAMESPACE = 'viewer.training.step_trace_viewer'
message = {"project_path": '',
           "device_id": "4",
           "sql_path": ''}

DB_TRACE = DBNameConstant.DB_TRACE
TABLE_TRAINING_TRACE = DBNameConstant.TABLE_TRAINING_TRACE
TABLE_ALL_REDUCE = DBNameConstant.TABLE_ALL_REDUCE


def create_trace_db():
    db_manager = DBManager()
    if os.path.exists(os.path.join(db_manager.db_path, DB_TRACE)):
        os.remove(os.path.join(db_manager.db_path, DB_TRACE))

    data1 = (('127.0.0.1', 4, 1, 65, 144, 1013979616375, 65, 145, 1013981208724,
              71, 189, 1013981571636, 64, 5, 1955261, 1592349, 362912, 0, 0),)

    data2 = (('127.0.0.1', 4, 1013981571636, 1013980654091, 70, 4, 1013981072280, 70, 103, 0),
             ('127.0.0.1', 4, 1013981571636, 1013981251417, 72, 6, 1013981476623, 72, 105, 0),)

    sql = "create table if not exists {0} (host_id int, device_id int, iteration_id int, " \
          "job_stream int, job_task int, FP_start int, FP_stream int, FP_task int, " \
          "BP_end int, BP_stream int, BP_task int, iteration_end int, iter_stream int," \
          "iter_task int, iteration_time int, fp_bp_time int, grad_refresh_bound int, " \
          "data_aug_bound int default 0, model_id int default 0)".format(TABLE_TRAINING_TRACE)
    insert_sql = "insert into {0} values ({value})".format(TABLE_TRAINING_TRACE, value="?," * (len(data1[0]) - 1) + "?")
    conn, cur = db_manager.create_table(DB_TRACE, sql, insert_sql, data1)
    conn.close()

    sql = "create table if not exists {0}(host_id int, device_id int," \
          "iteration_end int, start int, start_stream int, start_task int," \
          "end int, end_stream int, end_task int, model_id int default 0)".format(
        TABLE_ALL_REDUCE)
    insert_sql = "insert into {0} values ({value})".format(TABLE_ALL_REDUCE, value="?," * (len(data2[0]) - 1) + "?")
    conn, cur = db_manager.create_table(DB_TRACE, sql, insert_sql, data2)

    return db_manager, conn, cur


class TestStepTraceViewer(unittest.TestCase):

    def test_query_top_total_1(self):
        message_1 = None
        res = StepTraceViewer.query_top_total(message_1)
        self.assertEqual(res, {})

    def test_get_step_trace_summary(self):
        message1 = {'job_id': 'job_default', 'device_id': '4'}
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(hwts_frequency='100').device_info])).process()
        with mock.patch(NAMESPACE + '.StepTraceViewer.get_step_trace_data', side_effect=ValueError), \
                mock.patch(NAMESPACE + '.logging.error'):
            res = StepTraceViewer.get_step_trace_summary(message1)
        self.assertEqual(res, ([], [], 0))

        message1 = {'job_id': 'job_default', 'device_id': '4', 'project_path': ''}
        db_manager, conn, curs = create_trace_db()
        conn.close()

        with mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=db_manager.db_path):
            res = StepTraceViewer.get_step_trace_summary(message1)

        if os.path.exists(db_manager.db_name):
            os.remove(db_manager.db_name)
        self.assertEqual(res[2], 1)

    def test_step_trace_timeline(self):
        res = StepTraceViewer.get_trace_timeline_data(0, [0])
        self.assertTrue(isinstance(res, EmptyClass))

        message1 = {'job_id': 'job_default', 'device_id': '4', 'project_path': ''}
        db_manager, conn, curs = create_trace_db()
        conn.close()
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(hwts_frequency='100').device_info])).process()
        with mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=db_manager.db_path):
            res = StepTraceViewer.get_step_trace_timeline(message1)

        if os.path.exists(db_manager.db_name):
            os.remove(db_manager.db_name)
        self.assertEqual(len(res), 1865)

    def test_get_one_iter_timeline_data(self):
        db_manager, conn, curs = create_trace_db()
        conn.close()
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(hwts_frequency='100').device_info])).process()
        ProfilingScene().init("")
        with mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=db_manager.db_path), \
                mock.patch('common_func.utils.Utils.get_scene', return_value=Constant.STEP_INFO):
            res = StepTraceViewer.get_one_iter_timeline_data("", 1, 0)

        db_manager.conn.close()

        self.assertEqual(len(res), 1865)


if __name__ == '__main__':
    unittest.main()
