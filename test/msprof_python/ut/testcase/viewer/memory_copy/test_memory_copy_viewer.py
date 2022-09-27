import unittest
from unittest import mock
from viewer.memory_copy.memory_copy_viewer import MemoryCopyViewer
from common_func.memcpy_constant import MemoryCopyConstant
from common_func.ms_constant.str_constant import StrConstant

NAMESPACE = 'msmodel.memory_copy.memcpy_model.MemcpyModel'


class TestMemoryCopyViewer(unittest.TestCase):
    def setUp(self) -> None:
        self.current_path = ""
        self.memcopy_viewer = MemoryCopyViewer(self.current_path)

    def test_get_memory_copy_chip0_summary_1(self):
        expect_res = [(0, 20000, 1, 20000, 20000, 20000, 100, 20000, MemoryCopyConstant.DEFAULT_VIEWER_VALUE,
                       MemoryCopyConstant.TYPE, StrConstant.AYNC_MEMCPY, 1,
                       MemoryCopyConstant.DEFAULT_VIEWER_VALUE, 1)]

        export_data = [(20000, MemoryCopyConstant.TYPE, 1, 1, 100, 20000, 20000, 20000, 1)]
        with mock.patch(NAMESPACE + '.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.return_chip0_summary', return_value=export_data):
            res = self.memcopy_viewer.get_memory_copy_chip0_summary()

        self.assertEqual(expect_res, res)

    def test_get_memory_copy_chip0_summary_2(self):
        expect_res = []

        export_data = []
        with mock.patch(NAMESPACE + '.check_db', return_value=False), \
                mock.patch(NAMESPACE + '.return_chip0_summary', return_value=export_data):
            res = self.memcopy_viewer.get_memory_copy_chip0_summary()

        self.assertEqual(expect_res, res)

    def test_get_memory_copy_non_chip0_summary_1(self):
        expect_res = [("MemcopyAsync", "other", 11, 12, 100, '"2200"', '"2300"')]

        export_data = [(MemoryCopyConstant.ASYNC_MEMCPY_NAME, MemoryCopyConstant.TYPE, 11,
                        12, 100, 2.2, 2.3)]
        with mock.patch(NAMESPACE + '.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.return_not_chip0_summary', return_value=export_data):
            res = self.memcopy_viewer.get_memory_copy_non_chip0_summary()

        self.assertEqual(expect_res, res)

    def test_get_memory_copy_non_chip0_summary_2(self):
        expect_res = []
        export_data = []
        with mock.patch(NAMESPACE + '.check_db', return_value=False), \
                mock.patch(NAMESPACE + '.return_not_chip0_summary', return_value=export_data):
            res = self.memcopy_viewer.get_memory_copy_non_chip0_summary()

        self.assertEqual(expect_res, res)
