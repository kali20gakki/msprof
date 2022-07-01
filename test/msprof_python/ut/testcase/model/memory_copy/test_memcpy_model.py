import os
import shutil
import unittest
from common_func.db_name_constant import DBNameConstant
from common_func.memcpy_constant import MemoryCopyConstant
from msmodel.memory_copy.memcpy_model import MemcpyModel

NAMESPACE = 'msmodel.memcopy_copy.'


class TestMemcpyModel(unittest.TestCase):
    def setup_class(self):
        self.current_path = "./memcpy_test/"
        self.file_folder = "sqlite/"
        if not os.path.exists(self.current_path):
            os.mkdir(self.current_path)
        if not os.path.exists(self.current_path + self.file_folder):
            os.mkdir(self.current_path + self.file_folder)

        self.memcpy_model = MemcpyModel(self.current_path,
                                        DBNameConstant.DB_MEMORY_COPY, [DBNameConstant.TABLE_TS_MEMCPY_CALCULATION])
        self.memcpy_model.init()

    def teardown_class(self):
        self.memcpy_model.finalize()
        if os.path.exists(self.current_path):
            shutil.rmtree(self.current_path)

    def setUp(self):
        # stream_id, task_id, receive_time, start_time, end_time, duration, name, type
        memcpy_data = [(11, 12, 1000, 2000, 3000, 1000,
                        MemoryCopyConstant.ASYNC_MEMCPY_NAME, MemoryCopyConstant.TYPE)]
        self.memcpy_model.create_table()
        self.memcpy_model.flush(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION,
                                memcpy_data)

    def test_flush(self):
        data_list = [(11, 12, 1000, 2000, 3000, 1000,
                        MemoryCopyConstant.ASYNC_MEMCPY_NAME, MemoryCopyConstant.TYPE)]
        self.memcpy_model.create_table()
        self.memcpy_model.flush(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION, data_list)
        res = self.memcpy_model.get_all_data(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION)
        self.assertEqual(data_list, res)

    def test_export_chip0_summary(self):
        expect_res = [(1000, MemoryCopyConstant.TYPE,
                       12, 11, 1000, 1000, 1000, 1000, 1)]
        res = self.memcpy_model.return_chip0_summary(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION)
        self.assertEqual(expect_res, res)

    def test_export_non_chip0_summary(self):
        expect_res = [(MemoryCopyConstant.ASYNC_MEMCPY_NAME, MemoryCopyConstant.TYPE, 11,
                       12, 1000, 2000, 3000)]
        res = self.memcpy_model.return_not_chip0_summary(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION)
        self.assertEqual(expect_res, res)

    def test_export_task_scheduler_timeline(self):
        expect_res = [(MemoryCopyConstant.ASYNC_MEMCPY_NAME,
                      MemoryCopyConstant.TYPE,
                      1000,
                      2000,
                      3000,
                      1000,
                      11,
                      12)]
        res = self.memcpy_model.return_task_scheduler_timeline(DBNameConstant.TABLE_TS_MEMCPY_CALCULATION)
        self.assertEqual(expect_res, res)