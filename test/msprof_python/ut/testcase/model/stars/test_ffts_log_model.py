from unittest import mock

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.stars.ffts_log_model import FftsLogModel

MODELNAMESPACE = 'msmodel.stars.ffts_log_model'


class TestFftsLogModel(TestDirCRBaseModel):

    def test_get_ffts_task_data(self):
        with mock.patch(MODELNAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_ffts_task_data()
        self.assertEqual(ret, [1])

    def test_get_timeline_data(self):
        with mock.patch(MODELNAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_timeline_data()
        self.assertEqual([], ret)
        thread_data_list = ([0, 1, 2, 3, 4, 5, 6, 7, 8],)
        subtask_data_list = ([5, 6, 7, 8, 9, 1, 2, 4, 6],)
        with mock.patch(MODELNAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(MODELNAMESPACE + '.FftsLogModel._get_thread_time_data', return_value=thread_data_list), \
                mock.patch(MODELNAMESPACE + '.FftsLogModel._get_subtask_time_data', return_value=subtask_data_list):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_timeline_data()
        self.assertEqual([{'acsq_task_list': [], 'subtask_data_list': ([5, 6, 7, 8, 9, 1, 2, 4, 6],),
                           'thread_data_list': ([0, 1, 2, 3, 4, 5, 6, 7, 8],)}], ret)

    def test_get_summary_data(self):
        with mock.patch(MODELNAMESPACE + '.FftsLogModel.get_all_data', return_value=[]):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_summary_data()
        self.assertEqual(ret, [])

    def test_get_thread_time_data(self):
        with mock.patch(MODELNAMESPACE + '.FftsLogModel.get_all_data', return_value=[]):
            check = FftsLogModel('test', 'test', 'test')
            ret = check._get_thread_time_data()
        self.assertEqual(ret, [])

    def test_get_ffts_plus_sub_task_data_within_time_range_by_different_time_staggered_situation(self):
        ffts_plus_data = [
            [0, 23, 2, "AIV", "FFTS+", 1000, 2000, 1000, 0],
            [1, 23, 2, "AIC", "FFTS+", 3000, 4000, 1000, 0],
            [2, 23, 2, "AIC", "FFTS+", 5000, 6000, 1000, 0],
            [0, 24, 2, "MIX_AIV", "FFTS+", 7000, 8000, 1000, 0],
        ]
        model = FftsLogModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_SUBTASK_TIME])
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_SUBTASK_TIME, ffts_plus_data)

        device_tasks = model.get_ffts_plus_sub_task_data_within_time_range(3500, 4500)
        self.assertEqual(len(device_tasks), 1)

        device_tasks = model.get_ffts_plus_sub_task_data_within_time_range(0, 6000)
        self.assertEqual(len(device_tasks), 3)

        device_tasks = model.get_ffts_plus_sub_task_data_within_time_range(0, 4999)
        self.assertEqual(len(device_tasks), 2)

        device_tasks = model.get_ffts_plus_sub_task_data_within_time_range(7800, 8000)
        self.assertEqual(len(device_tasks), 1)

        device_tasks = model.get_ffts_plus_sub_task_data_within_time_range(9000, 10000)
        self.assertEqual(len(device_tasks), 0)
        model.finalize()

    def test_get_ffts_log_data_should_return_all_data(self: any) -> None:
        ffts_plus_data = [
            [0, 23, 1, 0, "AIC", "FFTS+", '100010', 38140480339123],
            [0, 24, 2, 1, "AIV", "FFTS+", '100010', 38140480339683],
            [0, 23, 1, 0, "AIC", "FFTS+", '100011', 38140480340343],
            [0, 24, 2, 1, "AIV", "FFTS+", '100011', 38140480398243],
            [0, 25, 1, 0, "MIX_AIV", "FFTS+", '100010', 38140480398643],
        ]
        with FftsLogModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_FFTS_LOG]) as model:
            model.insert_data_to_db(DBNameConstant.TABLE_FFTS_LOG, ffts_plus_data)
            ffts_log_dto = model.get_ffts_log_data()
            self.assertEqual(len(ffts_log_dto), 5)
            self.assertEqual(ffts_log_dto[0].task_time, 38140480339123)
