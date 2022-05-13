import unittest
from unittest import mock
from sqlite.db_manager import DBManager
from host_prof.host_cpu_usage.model.host_cpu_usage import HostCpuUsage

NAMESPACE = 'host_prof.host_cpu_usage.model.host_cpu_usage'


class TsetHostCpuUsage(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_insert_cpu_info_data(self):
        cpu_info = {}
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostCpuUsage(self.result_dir)
            check.insert_cpu_info_data(cpu_info)

    def test_insert_process_usage_data(self):
        process_usage_info = {}
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostCpuUsage(self.result_dir)
            check.insert_process_usage_data(process_usage_info)

    def test_insert_cpu_usage_data(self):
        cpu_usage_info = {}
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostCpuUsage(self.result_dir)
            check.insert_cpu_usage_data(cpu_usage_info)

    def test_has_cpu_usage_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist'):
            check = HostCpuUsage(self.result_dir)
            check.has_cpu_usage_data()

    def test_has_cpu_info_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist'):
            check = HostCpuUsage(self.result_dir)
            check.has_cpu_info_data()

    def test_get_cpu_list(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HostCpuUsage(self.result_dir)
            check.get_cpu_list()

    def test_get_cpu_info_data(self):
        create_sql = "create table if not exists CpuInfo (cpu_num INTEGER,clk_jiffies INTEGER)"
        insert_sql = "insert into {} values (?,?)".format('CpuInfo')
        data = ((20, 100),)
        db_manager = DBManager()
        res = db_manager.create_table("host_cpu_usage.db", create_sql, insert_sql, data)
        check = HostCpuUsage(self.result_dir)
        check.cur = res[1]
        result_1 = check.get_cpu_info_data()
        self.assertEqual(result_1, {"cpu_num": 20})
        res[1].execute("delete from CpuInfo where cpu_num=20")
        result_2 = check.get_cpu_info_data()
        self.assertEqual(result_2, {"cpu_num": 0})
        res[1].execute("drop table CpuInfo")
        db_manager.destroy(res)

    def test_get_cpu_usage_data(self):
        create_sql = "create table if not exists CpuUsage (start_time text,end_time text,cpu_no text,usage REAL)"
        insert_sql = "insert into {} values (?,?,?,?)".format('CpuUsage')
        data = ((10000, 20000, 20, 100),)
        db_manager = DBManager()
        res = db_manager.create_table("host_cpu_usage.db", create_sql, insert_sql, data)
        per_cpu_usage = res[1].execute("SELECT * FROM CpuUsage order by CAST(cpu_no AS DECIMAL)")
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=per_cpu_usage):
            check = HostCpuUsage(self.result_dir)
            result_1 = check.get_cpu_usage_data()
        self.assertEqual(result_1, [['CPU 20', 10.0, {'Usage(%)': 100.0}]])
        res[1].execute("drop table CpuUsage")
        db_manager.destroy(res)


if __name__ == '__main__':
    unittest.main()
