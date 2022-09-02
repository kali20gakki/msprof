import unittest
from unittest import mock
from host_prof.host_syscall.model.host_syscall import HostSyscall

NAMESPACE = 'host_prof.host_syscall.model.host_syscall'


class TsetHostSyscall(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_flush_data(self):
        with mock.patch(NAMESPACE + '.DBManager.executemany_sql'):
            check = HostSyscall(self.result_dir)
            check.cache_data = [['MSVP_UploaderD', 18070, '18072', 'nanosleep', '191449517.503',
                                 1057.0, '191449518.56', 191454818891.872, 191454819948.872]]
            check.flush_data()

    def test_has_runtime_api_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = HostSyscall(self.result_dir)
            result = check.has_runtime_api_data()
        self.assertEqual(result, True)

    def test_get_summary_runtime_api_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=(1, 2, 3, 4)):
            check = HostSyscall(self.result_dir)
            result = check.get_summary_runtime_api_data()
        self.assertEqual(result, (1, 2, 3, 4))

    def test_get_runtime_api_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=(1, 2, 3, 4)):
            check = HostSyscall(self.result_dir)
            result = check.get_runtime_api_data()
        self.assertEqual(result, (1, 2, 3, 4))

    def test_get_all_tid(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=(1, 2, 3, 4)):
            check = HostSyscall(self.result_dir)
            result = check.get_all_tid()
        self.assertEqual(result, (1, 2, 3, 4))


if __name__ == '__main__':
    unittest.main()
