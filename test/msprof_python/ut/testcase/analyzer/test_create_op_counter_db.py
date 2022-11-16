import sqlite3
import unittest
from unittest import mock

import pytest

from analyzer.create_op_counter_db import MergeOPCounter
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from common_func.platform.chip_manager import ChipManager
from constant.constant import CONFIG
from constant.ut_db_name_constant import DB_OP_COUNTER
from constant.ut_db_name_constant import TABLE_OP_COUNTER_GE_MERGE
from constant.ut_db_name_constant import TABLE_OP_COUNTER_OP_REPORT
from constant.ut_db_name_constant import TABLE_OP_COUNTER_RTS_TASK
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen

NAMESPACE = 'analyzer.create_op_counter_db'


class TestMergeOPCounter(unittest.TestCase):

    def test_is_db_need_to_create(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path',
                        return_value='test\\db'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db',
                           return_value=True):
            check = MergeOPCounter(CONFIG)
            result = check._is_db_need_to_create()
        self.assertEqual(result, True)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path',
                        return_value='test\\db'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db',
                           return_value=False), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path',
                           return_value='test\\db'):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check = MergeOPCounter(CONFIG)
            result = check._is_db_need_to_create()
        self.assertEqual(result, False)

    def test_init_params(self):
        with mock.patch(NAMESPACE + '.PathManager.get_sql_dir',
                        return_value='test\\test'):
            with pytest.raises(ProfException) as error:
                check = MergeOPCounter(CONFIG)
                check._init_params()
                self.assertEqual(error.value.code, 10)
            with mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.join', return_value='test\\test'), \
                    mock.patch('os.remove'):
                check = MergeOPCounter(CONFIG)
                check._init_params()

    def test_create_db(self):
        db_manager = DBManager()
        res = db_manager.create_table('op_counter.db')
        ge_create_sql = "CREATE TABLE IF NOT EXISTS ge_task_merge(model_name text,model_id INTEGER," \
                        "op_name text,op_type text,task_id INTEGER,stream_id INTEGER,device_id INTEGER)"
        rts_task_create_sql = "CREATE TABLE IF NOT EXISTS rts_task(model_id INTEGER,task_id INTEGER," \
                              "stream_id INTEGER,task_type INTEGER,duration INTEGER,device_id INTEGER)"
        op_report_create_sql = " CREATE TABLE IF NOT EXISTS op_report(model_name text,op_type text," \
                               "core_type text,occurrences text,total_time REAL,min REAL,avg REAL," \
                               "max REAL,ratio text,device_id INTEGER)"
        with mock.patch('os.path.join', return_value='test\\test'):
            with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                            return_value=(None, None)), \
                    mock.patch(NAMESPACE + '.logging.error'), \
                    pytest.raises(ProfException) as error:
                check = MergeOPCounter(CONFIG)
                check.create_db('test')
                self.assertEqual(error.value.code, 10)
            with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                            return_value=res):
                with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                                return_value=ge_create_sql):
                    check = MergeOPCounter(CONFIG)
                    check.create_db('test')
                with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                                return_value=rts_task_create_sql):
                    check = MergeOPCounter(CONFIG)
                    check.create_db('test')
                with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                                return_value=op_report_create_sql):
                    check = MergeOPCounter(CONFIG)
                    check.create_db('test')
        res[1].execute("drop table ge_task_merge")
        res[1].execute("drop table rts_task")
        res[1].execute("drop table op_report")
        db_manager.destroy(res)

    def test_get_ge_data(self):
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iter_list_with_index_and_model',
                        return_value={}), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = MergeOPCounter(CONFIG)
            result = check._get_ge_data('')
        self.assertEqual(result, [])

    def test_get_op_report_sql(self):
        with mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                        return_value="step_info"):
            check = MergeOPCounter(CONFIG)
            ProfilingScene().init('')
            result = check._get_op_report_sql()
        self.assertEqual("select op_type, ge_task_merge.task_type, count(op_type), sum(duration) as total_time, "
                         "min(duration) as min, sum(duration)/count(op_type) as avg, max(duration) as max, "
                         "ge_task_merge.model_id from ge_task_merge, rts_task "
                         "where ge_task_merge.task_id=rts_task.task_id and ge_task_merge.stream_id=rts_task.stream_id "
                         "and ge_task_merge.batch_id=rts_task.batch_id "
                         "and (ge_task_merge.context_id=rts_task.subtask_id "
                         "or (ge_task_merge.context_id=4294967295 and subtask_id=0)) "
                         "group by op_type,ge_task_merge.task_type",
                         result)

    def test_create_ge_merge(self):
        with mock.patch('os.path.join', return_value='test\\db'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                           return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.MergeOPCounter._get_ge_data', return_value=[[1]]), \
                mock.patch(NAMESPACE + '.DBManager.executemany_sql'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = MergeOPCounter(CONFIG)
            check._create_ge_merge()

    def test_create_task(self):
        InfoConfReader()._info_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.GetOpTableTsTime.get_task_time_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.insert_data_into_table'):
            ProfilingScene().init("")
            ProfilingScene()._scene = Constant.SINGLE_OP
            check = MergeOPCounter(CONFIG)
            check._create_task()
        with mock.patch(NAMESPACE + '.DBManager.insert_data_into_table', side_effect=sqlite3.Error), \
                mock.patch(NAMESPACE + '.GetOpTableTsTime.get_task_time_data', return_value=[]), \
                mock.patch(NAMESPACE + '.logging.error', return_value=[]):
            ProfilingScene().init("")
            ProfilingScene()._scene = Constant.STEP_INFO
            check = MergeOPCounter(CONFIG)
            check._create_task()

    def test_create_report(self):
        sql = "select op_type, task_type, count(op_type), sum(duration) as total_time," \
              " min(duration) as min, sum(duration)/count(op_type) as avg, max(duration) as max, " \
              "ge_task_merge.device_id, model_name from ge_task_merge, rts_task " \
              "where ge_task_merge.task_id=rts_task.task_id " \
              "and ge_task_merge.stream_id=rts_task.stream_id " \
              "and ge_task_merge.model_id=rts_task.model_id " \
              "and ge_task_merge.device_id=0 " \
              "and rts_task.device_id=0 " \
              "group by model_name,op_type"
        ge_create_sql = "CREATE TABLE IF NOT EXISTS ge_task_merge(model_name text,model_id INTEGER," \
                        "op_name text,op_type text,task_id INTEGER,stream_id INTEGER,device_id INTEGER)"

        data = ((1, 2, 1, 2, 2, 3, 1),)
        task_sql = "CREATE TABLE IF NOT EXISTS rts_task(model_id INTEGER,task_id INTEGER," \
                   "stream_id INTEGER,task_type INTEGER,duration INTEGER,device_id INTEGER)"
        task_data = ((2, 2, 3, 2, 2, 0),)
        with DBOpen(DB_OP_COUNTER) as db_open:
            db_open.create_table(ge_create_sql)
            db_open.insert_data(TABLE_OP_COUNTER_GE_MERGE, data)

            db_open.create_table(task_sql)
            db_open.insert_data(TABLE_OP_COUNTER_RTS_TASK, task_data)
            with mock.patch(NAMESPACE + '.MergeOPCounter._get_op_report_sql', return_value=sql):
                check = MergeOPCounter(CONFIG)
                check.conn, check.curs = db_open.db_conn, db_open.db_curs
                result = check._create_report()
                self.assertEqual(result, None)
            db_open.db_curs.execute("insert into ge_task_merge values(1, 2, 1, 2, 2, 3, 0)")
            db_open.db_curs.execute("CREATE TABLE IF NOT EXISTS op_report(model_name text,op_type text,"
                                    "core_type text,occurrences text,total_time REAL,min REAL,avg REAL,"
                                    "max REAL,ratio text)")
            with mock.patch(NAMESPACE + '.MergeOPCounter._get_op_report_sql', return_value=sql), \
                    mock.patch(NAMESPACE + '.get_ge_model_name_dict', return_value={0: "model_1"}):
                check = MergeOPCounter(CONFIG)
                check.conn, check.curs = db_open.db_conn, db_open.db_curs
                check._create_report()
                db_open.db_conn.commit()
                model_name = \
                    db_open.db_curs.execute("select model_name from {}".format(TABLE_OP_COUNTER_OP_REPORT)).fetchone()[
                        0]
                self.assertEqual("model_1", model_name)

    def test_run(self):
        db_manager = DBManager()
        res = db_manager.create_table('op_counter.db')
        with mock.patch(NAMESPACE + '.MergeOPCounter._init_params'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            with mock.patch(NAMESPACE + '.MergeOPCounter._is_db_need_to_create',
                            return_value=False), \
                    mock.patch(NAMESPACE + '.logging.warning'):
                check = MergeOPCounter(CONFIG)
                result = check.run()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.MergeOPCounter._is_db_need_to_create',
                            return_value=True), \
                    mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                               return_value="step_info"):
                with mock.patch(NAMESPACE + '.MergeOPCounter.create_db',
                                return_value=res), \
                        mock.patch(NAMESPACE + '.MergeOPCounter._create_ge_merge'), \
                        mock.patch(NAMESPACE + '.MergeOPCounter._create_task'), \
                        mock.patch(NAMESPACE + '.MergeOPCounter._create_report'):
                    check = MergeOPCounter(CONFIG)
                    check.run()
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.MergeOPCounter._init_params', side_effect=OSError):
            check = MergeOPCounter(CONFIG)
            check.run()
