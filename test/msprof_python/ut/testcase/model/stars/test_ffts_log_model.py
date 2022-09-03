import unittest
from unittest import mock

from constant.ut_db_name_constant import DB_SOC_LOG
from constant.ut_db_name_constant import TABLE_THREAD_TASK
from msmodel.stars.ffts_log_model import FftsLogModel
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.stars.ffts_log_model'


class StarsCommon:
    stream_id = 1
    task_id = 2
    timestamp = 3


class DataConstant:
    stars_common = StarsCommon
    subtask_id = 4
    thread_id = 5
    subtask_type = 6
    ffts_type = 0
    func_type = 7


class TestFftsLogModel(unittest.TestCase):

    def test_insert_log_data(self):
        with mock.patch(NAMESPACE + '.FftsLogModel.insert_data_to_db'), \
             mock.patch(NAMESPACE + '.FftsLogModel._FftsLogModel__create_log_table'):
            check = FftsLogModel('test', 'test', 'test')
            check.insert_log_data((DataConstant,))

    def test_flush(self):
        with mock.patch(NAMESPACE + '.FftsLogModel.insert_log_data'):
            check = FftsLogModel('test', 'test', 'test')
            check.flush(1)

    def test_get_ffts_task_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_ffts_task_data()
        self.assertEqual(ret, [1])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_timeline_data()
        self.assertEqual({}, ret)
        thread_data_list = ([0, 1, 2, 3, 4, 5, 6, 7, 8],)
        subtask_data_list = ([5, 6, 7, 8, 9, 1, 2, 4, 6],)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.FftsLogModel._get_thread_time_data', return_value=thread_data_list), \
             mock.patch(NAMESPACE + '.FftsLogModel._get_subtask_time_data', return_value=subtask_data_list):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_timeline_data()
        self.assertEqual({'subtask_data_list': ([5, 6, 7, 8, 9, 1, 2, 4, 6],),
                          'thread_data_list': ([0, 1, 2, 3, 4, 5, 6, 7, 8],)}, ret)

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + '.FftsLogModel.get_all_data', return_value=1):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_summary_data()
        self.assertEqual(ret, 1)

    def test_get_thread_time_data(self):
        with mock.patch(NAMESPACE + '.FftsLogModel.get_all_data', return_value=1):
            check = FftsLogModel('test', 'test', 'test')
            ret = check._get_thread_time_data()
        self.assertEqual(ret, 1)

    def test_get_subtask_time_data(self):
        with mock.patch(NAMESPACE + '.FftsLogModel.get_all_data', return_value=1):
            check = FftsLogModel('test', 'test', 'test')
            ret = check._get_subtask_time_data()
        self.assertEqual(ret, 1)

    def test_get_thread_task_time_sql(self):
        sql = "Select end_log.subtask_id, end_log.task_id,end_log.stream_id,end_log.subtask_type, " \
              "end_log.ffts_type,end_log.thread_id,start_log.task_time," \
              "(end_log.task_time-start_log.task_time) as dur_time " \
              "from FftsLog end_log " \
              "join FftsLog start_log on end_log.thread_id=start_log.thread_id " \
              "and end_log.subtask_id=start_log.subtask_id and end_log.stream_id=start_log.stream_id " \
              "and end_log.task_id=start_log.task_id " \
              "where end_log.task_type='100011' and start_log.task_type='100010' " \
              "group by end_log.subtask_id,end_log.thread_id, end_log.task_id " \
              "order by end_log.subtask_id"
        check = FftsLogModel('test', 'test', 'test')
        ret = check._get_thread_task_time_sql()
        self.assertEqual(ret, sql)

    def test_get_subtask_time_sql(self):
        sql = "Select end_log.subtask_id, end_log.task_id,end_log.stream_id,end_log.subtask_type,end_log.ffts_type," \
              "min(start_log.task_time) as start_time, (max(end_log.task_time)-min(start_log.task_time)) as dur_time " \
              "from FftsLog end_log " \
              "join FftsLog start_log on end_log.subtask_id=start_log.subtask_id " \
              "and end_log.stream_id=start_log.stream_id and end_log.task_id=start_log.task_id " \
              "where end_log.task_type='100011' and start_log.task_type='100010' " \
              "group by end_log.subtask_id, end_log.task_id " \
              "order by start_time"
        check = FftsLogModel('test', 'test', 'test')
        ret = check._get_subtask_time_sql()
        self.assertEqual(ret, sql)

    def test_create_log_table(self):
        sql = "Select end_log.subtask_id, end_log.task_id,end_log.stream_id,end_log.subtask_type, " \
              "end_log.ffts_type,end_log.thread_id,start_log.task_time," \
              "(end_log.task_time-start_log.task_time) as dur_time " \
              "from FftsThreadLog end_log join FftsThreadLog start_log " \
              "on end_log.thread_id=start_log.thread_id and end_log.subtask_id=start_log.subtask_id " \
              "and end_log.stream_id=start_log.stream_id where end_log.task_type='100011' " \
              "and start_log.task_type='100010' group by end_log.subtask_id,end_log.thread_id " \
              "order by end_log.subtask_id"
        db_manager = DBManager()
        conn, curs = db_manager.create_table(DB_SOC_LOG)
        curs.execute('CREATE TABLE if not exists FftsThreadLog(stream_id INTEGER,task_id INTEGER,subtask_id INTEGER,'
                     'thread_id TEXT,subtask_type INTEGER,ffts_type INTEGER,task_type TEXT,task_time INTEGER)')
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.drop_table'):
            check = FftsLogModel('test', 'test', 'test')
            check.cur = curs
            check._FftsLogModel__create_log_table(TABLE_THREAD_TASK, sql)
        curs.execute('drop table FftsThreadLog')
        curs.execute('drop table ThreadTime')
        db_manager.destroy((conn, curs))
