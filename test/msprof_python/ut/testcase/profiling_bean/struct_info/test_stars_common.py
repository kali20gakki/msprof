import unittest
from profiling_bean.stars.stars_common import StarsCommon

NAMESPACE = 'profiling_bean.struct_info.stars_common'


class TestStarsCommon(unittest.TestCase):

    def test_init(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        self.assertEqual(check._stream_id, 2)

    def test_task_id(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        ret = check.task_id
        self.assertEqual(ret, 1)

    def test_stream_id(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        self.assertEqual(check.stream_id, 2)

    def test_timestamp(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        self.assertEqual(check.timestamp, 3)


if __name__ == '__main__':
    unittest.main()
