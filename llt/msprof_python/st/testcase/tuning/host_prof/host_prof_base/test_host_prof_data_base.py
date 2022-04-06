import unittest
import numpy
from unittest import mock
from sqlite.db_manager import DBManager
from host_prof.host_prof_base.host_prof_data_base import HostProfDataBase

NAMESPACE = 'host_prof.host_prof_base.host_prof_data_base'


class TsetHostProfDataBase(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_init(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                           return_value=(None, None)):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            result = check.init()
        self.assertEqual(result, False)
        db_manager = DBManager()
        res = db_manager.create_table('host_cpu_usage.db')
        res[1].execute('create table if not exists CpuInfo(id integer, name integer)')
        sql = "create table if not exists CpuUsage(id integer, name integer)"
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                           return_value=res), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                           return_value=sql):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo', 'CpuUsage'])
            result = check.init()
        self.assertEqual(result, True)
        res[1].execute("drop table CpuInfo")
        res[1].execute("drop table CpuUsage")
        db_manager.destroy(res)

    def test_check_db(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db',
                        return_value=(None, None)):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            result = check.check_db()
        self.assertEqual(result, False)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db',
                        return_value=(1, 1)):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            result = check.check_db()
        self.assertEqual(result, True)

    def test_insert_single_data(self):
        data = [191424.758430371, 191424.778907458, '0.000000']
        with mock.patch(NAMESPACE + '.HostProfDataBase.flush_data', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.exception'):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            check.cache_data = list(numpy.arange(10001))
            check.insert_single_data(data)
        self.assertEqual(check.cache_data, [])


if __name__ == '__main__':
    unittest.main()
