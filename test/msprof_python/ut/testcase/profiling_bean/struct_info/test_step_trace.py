import unittest
from profiling_bean.struct_info.step_trace import StepTrace

NAMESPACE = 'profiling_bean.struct_info.step_trace'
args = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]


class TestStepTrace(unittest.TestCase):
    def test_init(self):
        test = StepTrace(args)
        self.assertEqual(test.timestamp, 0)


if __name__ == '__main__':
    unittest.main()
