import unittest
from common_func.json_manager import JsonManager


class TestTraceViewer(unittest.TestCase):

    def test_loads_0(self):
        expect_res = {}
        res = JsonManager.loads(10)
        self.assertEqual(expect_res, res)

    def test_loads_1(self):
        expect_res = {}
        res = JsonManager.loads("123dsads")
        self.assertEqual(expect_res, res)

    def test_loads_2(self):
        expect_res = {'status': 2, 'info': 'Can not get aicore utilization'}
        res = JsonManager.loads('{"status": 2, "info": "Can not get aicore utilization"}')
        self.assertEqual(expect_res, res)
