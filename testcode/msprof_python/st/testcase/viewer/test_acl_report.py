import sqlite3
import unittest
from unittest import mock
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen
from viewer.acl_report import get_acl_data, get_acl_data_for_device, get_acl_statistic_data, \
    _get_get_acl_statistic_data

NAMESPACE = 'viewer.acl_report'
configs = {'handler': '_get_acl_data',
           'headers': ['Name', 'Type', 'Start Time', 'Duration(us)', 'Process ID', 'Thread ID'],
           'db': 'acl_module.db',
           'table': 'acl_data', 'unused_cols': []}


class TestAclReport(unittest.TestCase):
    def test_get_acl_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = get_acl_data('', '', 1, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.get_acl_data_for_device', return_value=[]):
            res = get_acl_data('', '', 1, configs)
        self.assertEqual(res[1], [])

    def test_get_acl_data_for_device_1(self):
        create_sql = "CREATE TABLE IF NOT EXISTS acl_data(api_name, api_type, start_time, end_time, " \
                     "process_id, thread_id, device_id)"
        data = (("aclmdlQuerySize", "model", 117962568517, 118405906350, 2244, 2246, 0),)
        with DBOpen(DBNameConstant.DB_ACL_MODULE) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_ACL_DATA, data)
            res = get_acl_data_for_device(db_open.db_curs, DBNameConstant.TABLE_ACL_DATA, 0)
            self.assertEqual(len(res), 1)

    def test_get_acl_data_for_device_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS acl_data(api_name, api_type, start_time, end_time, " \
                     "process_id, thread_id, device_id)"
        with DBOpen(DBNameConstant.DB_ACL_MODULE) as db_open:
            db_open.create_table(create_sql)
            res = get_acl_data_for_device(db_open.db_curs, "acl_data", 0)
            self.assertEqual(res, [])

    def test_get_acl_statistic_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_acl_statistic_data('', '', 1, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '._get_get_acl_statistic_data', return_value=[]):
            res = get_acl_statistic_data('', '', 1, configs)
        self.assertEqual(res[1], [])

    def test_get_get_acl_statistic_data_1(self):
        create_sql = "CREATE TABLE IF NOT EXISTS acl_data(api_name, api_type, start_time, end_time, " \
                     "process_id, thread_id, device_id)"
        data = (("aclmdlQuerySize", "model", 117962568517, 118405906350, 2244, 2246, 0),)
        with DBOpen(DBNameConstant.DB_ACL_MODULE) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_ACL_DATA, data)
            res = _get_get_acl_statistic_data(db_open.db_curs, "acl_data", 0)
            self.assertEqual(len(res), 1)

    def test_get_get_acl_statistic_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS acl_data(api_name, api_type, start_time, end_time, " \
                     "process_id, thread_id, device_id)"
        with DBOpen(DBNameConstant.DB_ACL_MODULE) as db_open:
            db_open.create_table(create_sql)
            res = _get_get_acl_statistic_data(db_open.db_curs, "acl_data", 0)
            self.assertEqual(res, [])


if __name__ == '__main__':
    unittest.main()
