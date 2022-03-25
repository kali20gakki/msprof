import sqlite3
import unittest
from collections import OrderedDict
from unittest import mock

import pytest

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from model.aic.ai_core_sample_model import AiCoreSampleModel

from sqlite.db_manager import DBManager

NAMESPACE = 'model.aic.ai_core_sample_model'


class TestAiCoreSampleModel(unittest.TestCase):

    def test_init(self):
        db_manager = DBManager()
        res = db_manager.create_table('aicore.db')
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=None), \
                mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(None, None)):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            ret = check.init()
        self.assertFalse(ret)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=None), \
                mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=res), \
                mock.patch(NAMESPACE + '.BaseModel.create_table'):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            ret = check.init()
        self.assertTrue(ret)
        db_manager.destroy(res)

    def test_create_aicore_originaldatatable(self):
        table_name = 'AICoreOriginalData'
        sql = "CREATE TABLE IF NOT EXISTS AICoreOriginalData(mode integer, replayid integer," \
              "timestamp integer, coreid integer, task_cyc integer, event1 integer, event2 integer," \
              " event3 integer, event4 integer)"
        db_manager = DBManager()
        res = db_manager.create_table("aicore.db")
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=None), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.error'), \
                pytest.raises(ProfException) as err:
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            check.conn = db_manager.create_table("aicore.db")[0]
            check.create_aicore_originaldatatable(table_name)
            self.assertEqual(err.value.code, 10)
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=sql):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            check.conn, check.cur = res[0], res[1]
            check.create_aicore_originaldatatable(table_name)
            check.conn.close()
        res = db_manager.connect_db("aicore.db")
        res[1].execute('drop table AICoreOriginalData')
        db_manager.destroy(res)

    def test_create_ai_vector_core_db(self):
        events = ['0x8', '0xa', '0x9', '0xb', '0xc', '0xd', '0x54', '0x55']
        db_manager = DBManager()
        res = db_manager.create_table('aicore.db')
        res[1].execute("CREATE TABLE IF NOT EXISTS AICoreOriginalData(mode integer, "
                       "replayid integer,timestamp integer, coreid integer, task_cyc integer,"
                       " event1 integer, event2 integer,event3 integer, event4 integer,"
                       "event5 integer, event6 integer,event7 integer, event8 integer)")
        res[1].execute("insert into AICoreOriginalData values "
                       "(0,0,0,0,0,0,0,0,0,0,0,0,0),"
                       "(1,1,1,1,1,1,1,1,1,1,1,1,1),"
                       "(2,2,2,2,2,2,2,2,2,2,2,2,2),"
                       "(3,3,3,3,3,3,3,3,3,3,3,3,3),"
                       "(4,4,4,4,4,4,4,4,4,4,4,4,4),"
                       "(5,5,5,5,5,5,5,5,5,5,5,5,5),"
                       "(6,6,6,6,6,6,6,6,6,6,6,6,6),"
                       "(7,7,7,7,7,7,7,7,7,7,7,7,7)")
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.AiCoreSampleModel.create_aicore_originaldatatable'), \
                mock.patch(NAMESPACE + '.AiCoreSampleModel.flush'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.logging.error'):
            with mock.patch(NAMESPACE + '.check_aicore_events', side_effect=OSError):
                check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
                check.create_ai_vector_core_db(events, [])

            with mock.patch(NAMESPACE + '.check_aicore_events'):
                check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
                check.conn, check.cur = res[0], res[1]
                check.create_ai_vector_core_db(events, [])

            with mock.patch(NAMESPACE + '.check_aicore_events'):
                with mock.patch(NAMESPACE + '.AiCoreSampleModel.get_ai_core_event_chunk',
                                return_value=None):
                    check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
                    check.conn, check.cur = res[0], res[1]
                    check.create_ai_vector_core_db(events, [])
        res[1].execute('drop table AICoreOriginalData')
        res[1].execute('drop table r55')
        res[1].execute('drop table r54')
        res[1].execute('drop table r8')
        res[1].execute('drop table r9')
        res[1].execute('drop table ra')
        res[1].execute('drop table rb')
        res[1].execute('drop table rc')
        res[1].execute('drop table rd')
        res[1].execute('drop table task_cyc')
        res[1].execute('drop table EventCount')
        db_manager.destroy(res)

    def test_insert_metric_summary_table(self):
        sql = 'SELECT cast(SUM((task_cyc*1000/(100))) as decimal(8,2)),' \
              'cast(1.0*SUM(r49)/SUM(task_cyc) as decimal(8,2)),' \
              'cast(1.0*SUM(r4a)/SUM(task_cyc) as decimal(8,2)),' \
              'r4b, r4c, r4d, r4e FROM EventCount where coreid = ?'
        create_sql = "CREATE TABLE IF NOT EXISTS EventCount (coreid integer, task_cyc integer," \
                     "r49 integer, r4a integer, r4b integer, r4c integer, r4d integer, r4e integer)"
        data = ((1, 1, 1, 1, 1, 1, 1, 1),)
        insert_sql = "insert into {} values (?,?,?,?,?,?,?,?)".format('EventCount')

        create_metric_sql = "CREATE TABLE IF NOT EXISTS MetricSummary (metric text, " \
                            "value numeric, coreid INT)"
        insert_metric_sql = "insert into {} values (?,?,?)".format('MetricSummary')
        metric_data = ((1, 1, 1),)
        db_manager = DBManager()
        db_manager.create_table("aicore.db", create_sql, insert_sql, data)
        res = db_manager.create_table("aicore.db", create_metric_sql, insert_metric_sql, metric_data)
        freq = 1150000000
        metric_key = 'test'
        check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
        result = check.insert_metric_summary_table(freq, metric_key)
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.AiCoreSampleModel.sql_insert_metric_summary_table',
                           return_value=sql):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            check.conn = res[0]
            check.cur = res[1]
            check.insert_metric_summary_table(freq, 'ArithmeticUtilization')
        res[0].commit()
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.AiCoreSampleModel._get_metrics', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.error'):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            check.insert_metric_summary_table(freq, 'ArithmeticUtilization')

    def test_insert_metric_value(self):
        metrics_config = OrderedDict([('total_cycles', 'task_cyc'),
                                      ('scalar_ld_ratio', '1.0*SUM(r3a)/SUM(task_cyc)')])
        create_sql = "CREATE TABLE IF NOT EXISTS EventCount (coreid integer, task_id integer )"
        insert_sql = "insert into {} values (?,?)".format('EventCount')
        data = ((1, 1),)
        db_manager = DBManager()
        res = db_manager.create_table("aicore.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=False):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            result = check.insert_metric_value()
        self.assertEqual(result, 1)
        with mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=metrics_config):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            check.conn = res[0]
            check.cur = res[1]
            check.insert_metric_value()
        res[1].execute("drop table EventCount")
        res[1].execute("drop table MetricSummary")
        db_manager.destroy(res)

    def test_sql_insert_metric_summary_table(self):
        metrics = ['total_time(ms)', "mac_fp16_ratio", "mac_int8_ratio"]
        freq = 100
        metric_key = 'ai_core_metrics'
        sql = 'SELECT cast(SUM((task_cyc*1000/(100))) as decimal(8,2)),' \
              'cast(1.0*SUM(r49)/SUM(task_cyc) as decimal(8,2)),' \
              'cast(1.0*SUM(r4a)/SUM(task_cyc) as decimal(8,2)) ' \
              'FROM EventCount where coreid = ?'
        with mock.patch(NAMESPACE + '.CalculateAiCoreData.add_fops_header'):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            result = check.sql_insert_metric_summary_table(metrics, freq, metric_key)
        self.assertEqual(result, sql)

    def test_get_ai_core_event_chunk(self):
        event = [1, 1, 1, 1, 1, 1, 1, 1]
        with mock.patch(NAMESPACE + '.check_aicore_events'):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'])
            result = check.get_ai_core_event_chunk(event)
        self.assertEqual(result, [[1, 1, 1, 1, 1, 1, 1, 1]])

    def test_flush(self):
        sql = "CREATE TABLE IF NOT EXISTS AICoreOriginalData(mode integer, replayid integer," \
              "timestamp integer, coreid integer, task_cyc integer, event1 integer, event2 integer," \
              " event3 integer, event4 integer)"
        db_manager = DBManager()
        res = db_manager.create_table('aicore.db')
        res[1].execute(sql)
        InfoConfReader()._info_json = {'devices': '0'}
        check = AiCoreSampleModel('test', 'nic.db', ['NicOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.flush([[1, 2, 3, 2, 3, 5, 6]])
        res[1].execute("drop table if exists AICoreOriginalData")
        db_manager.destroy(res)

    def test_clear(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'),\
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True),\
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = AiCoreSampleModel('test', 'nic.db', ['NicOriginalData'])
            check.clear()
