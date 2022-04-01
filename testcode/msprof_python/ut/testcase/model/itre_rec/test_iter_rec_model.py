import unittest
from unittest import mock

from model.iter_rec.iter_rec_model import HwtsIterModel
from sqlite.db_manager import DBManager
from sqlite.db_manager import CursorDemo

NAMESPACE = 'model.iter_rec.iter_rec_model'


class TestHwtsIterModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HwtsIterModel.insert_data_to_db'):
            check = HwtsIterModel('test')
            check.flush([], "HwtsIter")

    def test_check_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('HwtsIter.db')
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = HwtsIterModel('test')
            result = check.check_table()
            self.assertFalse(result)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = HwtsIterModel('test')
            check.conn, check.cur = res[0], res[1]
            result = check.check_table()
            self.assertTrue(result)
        db_manager.destroy(res)

    def test_get_task_offset_and_sum(self):
        db_manager = DBManager()
        create_sql = "CREATE TABLE if not exists HwtsIter(ai_core_num INTEGER,task_count INTEGER," \
                     "iter_id INTEGER,sys_cnt INTEGER)"
        data = [(1, 2, 1, 456), (1, 2, 0, 6)]
        insert_sql = "insert into HwtsIter values({value})".format(value="?," * (len(data[0]) - 1) + "?")
        res = db_manager.create_table('hwts.db', create_sql, insert_sql, data)
        check = HwtsIterModel('test')
        check.conn, check.cur = res[0], res[1]
        result = check.get_task_offset_and_sum(1, 'ai_core_num')
        self.assertEqual(result, (1, 1))
        res[1].execute('drop table if exists HwtsIter')
        with mock.patch(NAMESPACE + '.logging.error'):
            check = HwtsIterModel('test')
            check.conn, check.cur = res[0], res[1]
            result = check.get_task_offset_and_sum(1, 'ai_core_num')
            self.assertEqual(result, (0, 0))
        db_manager.destroy(res)

    def test_get_aic_sum_count(self):
        db_manager = DBManager()
        create_sql = "CREATE TABLE if not exists HwtsIter(ai_core_num INTEGER,task_count INTEGER," \
                     "iter_id INTEGER,sys_cnt INTEGER)"
        data = [(1, 2, 1, 456), (1, 2, 0, 6)]
        insert_sql = "insert into HwtsIter values({value})".format(value="?," * (len(data[0]) - 1) + "?")
        res = db_manager.create_table('hwts.db', create_sql, insert_sql, data)
        check = HwtsIterModel('test')
        check.conn, check.cur = res[0], res[1]
        result = check.get_aic_sum_count()
        self.assertEqual(result, 2)
        res[1].execute('drop table if exists HwtsIter')
        with mock.patch('common_func.db_manager' + '.logging.error'):
            check = HwtsIterModel('test')
            check.conn, check.cur = res[0], res[1]
            result = check.get_aic_sum_count()
            self.assertEqual(result, 0)
        db_manager.destroy(res)

    def test_get_batch_list(self: any):
        iter_id = 1
        table_name = "HwtsBatch"
        curs = mock.Mock()
        curs.execute.fetchall = [0, 1]

        check = HwtsIterModel('test')
        check.init()
        check.cur = curs
        check.get_batch_list(iter_id, table_name)
