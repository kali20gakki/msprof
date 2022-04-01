import unittest
from unittest import mock

from profiling_bean.stars.acsq_task import AcsqTask

NAMESPACE = 'profiling_bean.stars.acsq_task'


class TestAcsqTask(unittest.TestCase):

    def test_init(self):
        args = [1000, 2, 3, 4, 66, 7, 8, 9, 54]
        with mock.patch(NAMESPACE + '.Utils.get_func_type', return_value=1):
            check = AcsqTask(args)
            self.assertEqual(check.func_type, 1)


if __name__ == '__main__':
    unittest.main()
