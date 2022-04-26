import unittest
from unittest import mock

from model.task_time.task_time import TaskTime

NAMESPACE = 'model.task_time.task_time'


class TestTaskTime(unittest.TestCase):
    def test_init(self):
        check = TaskTime()
        result = (check.task_id, check.stream_id)
        self.assertEqual(result, (None, None))
        result = check.wait_time + check.model_id
        self.assertEqual(result, 0)

    def test_construct(self):
        with mock.patch(NAMESPACE + '.TaskTime._pre_check'):
            check = TaskTime()
            check.construct(1, 2, 3, 4, 5, 6, 7)
            self.assertEqual(check._task_id, 1)
