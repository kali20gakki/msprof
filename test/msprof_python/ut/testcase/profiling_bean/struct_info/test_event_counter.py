import unittest
from profiling_bean.struct_info.event_counter import CounterInfo, AiCoreTaskInfo

NAMESPACE = 'profiling_bean.struct_info.event_counter'


class TestCounterInfo(unittest.TestCase):
    def test_init(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0)
        self.assertEqual(test._overflow, 0)

    def test_overflow(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).overflow
        self.assertEqual(test, 0)

    def test_overflow_cycle(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).overflow_cycle
        self.assertEqual(test, 0)

    def test_task_cyc(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).task_cyc
        self.assertEqual(test, 0)

    def test_time_stamp(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).time_stamp
        self.assertEqual(test, 0)

    def test_block(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).block
        self.assertEqual(test, 0)

    def test_overflow_cycle_2(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).overflow_cycle
        self.assertEqual(test, 0)


class TestAiCoreTaskInfo(unittest.TestCase):
    def test_init(self):
        test = AiCoreTaskInfo([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
        self.assertEqual(test.task_type, 0)


if __name__ == '__main__':
    unittest.main()
