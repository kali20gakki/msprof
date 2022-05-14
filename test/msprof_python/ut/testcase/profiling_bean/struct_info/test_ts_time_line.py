import unittest
from profiling_bean.struct_info.ts_time_line import TimeLineData

NAMESPACE = 'profiling_bean.struct_info.step_trace'
args = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]


class TestStepTrace(unittest.TestCase):
    def test_init(self):
        test = TimeLineData(args)
        self.assertEqual(test.task_type, 0)


if __name__ == '__main__':
    unittest.main()
