import unittest
from unittest import mock

from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from sqlite.db_manager import DBManager
from sqlite.db_manager import CursorDemo

NAMESPACE = 'msmodel.iter_rec.iter_rec_model'


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
        create_sql = "CREATE TABLE if not exists HwtsIter(ai_core_num INTEGER, " \
                     "ai_core_offset INTEGER, " \
                     "task_count INTEGER, " \
                     "task_offset INTEGER, " \
                     "iter_id INTEGER," \
                     "model_id INTEGER," \
                     "index_id INTEGER)"
        data = [(1, 0, 3, 0, 1, 1, 1), (1, 1, 4, 3, 2, 1, 2)]
        insert_sql = "insert into HwtsIter values({value})".format(value="?," * (len(data[0]) - 1) + "?")
        res = db_manager.create_table('hwts.db', create_sql, insert_sql, data)
        check = HwtsIterModel('test')
        check.conn, check.cur = res[0], res[1]
        result = check.get_task_offset_and_sum(1, 1, 'ai_core')
        self.assertEqual(result, (0, 1))
        res[1].execute('drop table if exists HwtsIter')
        with mock.patch(NAMESPACE + '.logging.error'):
            check = HwtsIterModel('test')
            check.conn, check.cur = res[0], res[1]
            result = check.get_task_offset_and_sum(1, 1, 'ai_core')
            self.assertEqual(result, (0, 0))
        db_manager.destroy(res)

    def test_get_aic_sum_count(self):
        db_manager = DBManager()
        create_sql = "CREATE TABLE if not exists HwtsIter(ai_core_num INTEGER, " \
                     "ai_core_offset INTEGER, " \
                     "task_count INTEGER, " \
                     "task_offset INTEGER, " \
                     "iter_id INTEGER)"
        data = [(1, 0, 3, 0, 1), (1, 1, 4, 3, 2)]
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
        table_name = "HwtsBatch"
        check = HwtsIterModel('test')
        check.init()

        with mock.patch("common_func.db_manager.DBManager.fetch_all_data", return_value=[0, 1]):
            res = check.get_batch_list(table_name, 4, 2)
        self.assertEqual(res, [0, 1])
