import unittest
from common_func.batch_counter import BatchCounter


class TestFileManager(unittest.TestCase):

    def test_calibrate_initial_batch0(self):
        self.check = BatchCounter("")
        self.check._stream_batch_dict = {2: (3, 4)}

        expect_res = 0
        res = self.check.calibrate_initial_batch(3, 4)
        self.assertEqual(expect_res, res)

    def test_calibrate_initial_batch1(self):
        self.check = BatchCounter("")
        self.check._ge_task_batch_dict = {2: (3, 4)}

        expect_res = 4
        res = self.check.calibrate_initial_batch(2, 2)
        self.assertEqual(expect_res, res)

    def test_calibrate_initial_batch2(self):
        self.check = BatchCounter("")
        self.check._ge_task_batch_dict = {2: (3, 4)}

        expect_res = 3
        res = self.check.calibrate_initial_batch(2, 65534)
        self.assertEqual(expect_res, res)

    def test_calibrate_initial_batch3(self):
        self.check = BatchCounter("")
        self.check._ge_task_batch_dict = {2: (3, 4)}

        expect_res = 4
        res = self.check.calibrate_initial_batch(2, 2)
        res = self.check.calibrate_initial_batch(2, 5)
        self.assertEqual(expect_res, res)

    def test_deal_batch_id_for_each_task(self):
        self.check = BatchCounter("")
        stream_max_value = {2: {"task_id": 65535, "batch_id": 4}}

        expect_res = 5
        res = self.check.deal_batch_id_for_each_task(2, 4, stream_max_value, 4)
        self.assertEqual(expect_res, res)