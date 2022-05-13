import unittest
from host_prof.host_cpu_usage.model.cpu_time_info import CpuTimeInfo

NAMESPACE = 'host_prof.host_cpu_usage.model.cpu_time_info'


class TestCpuTimeInfo(unittest.TestCase):

    line_info = "0 a b 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 " \
                "25 26 27 28 29 30 31 32 33 34 35 36 37 38 39"

    def test_pid(self):
        check = CpuTimeInfo(self.line_info)
        result_pid = check.pid
        result_tid = check.tid
        result_utime = check.utime
        result_stime = check.stime
        result_cpu_no = check.cpu_no
        self.assertEqual(result_pid, 0)
        self.assertEqual(result_tid, 3)
        self.assertEqual(result_utime, 13)
        self.assertEqual(result_stime, 14)
        self.assertEqual(result_cpu_no, 38)


if __name__ == '__main__':
    unittest.main()
