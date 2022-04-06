import unittest
from profiling_bean.basic_info.cpu_info import CpuInfo

NAMESPACE = 'profiling_bean.basic_info.cpu_info'


class TestCpuInfo(unittest.TestCase):
    def test_init(self):
        test = CpuInfo(0, 10, 10, "cpu", "ai_cpu")
        self.assertEqual(test.cpu_id(), 0)


if __name__ == '__main__':
    unittest.main()
