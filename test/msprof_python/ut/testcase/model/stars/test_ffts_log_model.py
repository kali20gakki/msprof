import unittest
from unittest import mock

import pytest

from mscalculate.stars.sub_task_calculate import SubTaskCalculator
from msmodel.stars.ffts_log_model import FftsLogModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.stars.sub_task_calculate'
MODELNAMESPACE = 'msmodel.stars.ffts_log_model'


class StarsCommon:
    stream_id = 1
    task_id = 2
    timestamp = 3


class DataConstant:
    stars_common = StarsCommon
    subtask_id = 4
    thread_id = 5
    subtask_type = 6
    ffts_type = 0
    func_type = 7


class TestSubTaskCalculator(unittest.TestCase):
    sample_config = {"result_dir": "test"}

    def test_insert_log_data(self):
        with mock.patch(MODELNAMESPACE + '.FftsLogModel.insert_data_to_db'):
            check = FftsLogModel('test', 'test', ['test'])
            check.flush([DataConstant])

    def test_ms_run(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.SubTaskCalculator.calculate'), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.SubTaskCalculator.save'):
            check = SubTaskCalculator(file_list, self.sample_config)
            check.ms_run()
            check = SubTaskCalculator({}, self.sample_config)
            check.ms_run()

    def test_get_subtask_time_data(self):
        with mock.patch(MODELNAMESPACE + '.FftsLogModel.get_all_data', return_value=1):
            check = FftsLogModel('test', 'test', ['test'])
            ret = check._get_subtask_time_data()
        self.assertEqual(ret, 1)

    def test_calculate(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.SubTaskCalculator.save', return_value=[]):
            check = SubTaskCalculator(file_list, self.sample_config)
            check.calculate()
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.SubTaskCalculator.save', side_effect=ValueError):
            check = SubTaskCalculator(file_list, self.sample_config)
            check.calculate()

    def test_init(self: any) -> None:
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
            with pytest.raises(ValueError) as err:
                check = SubTaskCalculator(file_list, self.sample_config)
                check.init()

    def test_save(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            with pytest.raises(AttributeError) as err:
                check = SubTaskCalculator(file_list, self.sample_config)
                check.save()

    def test_get_thread_task_time_sql(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        sql = "Select end_log.subtask_id, end_log.task_id,end_log.stream_id,end_log.subtask_type, " \
              "end_log.ffts_type,end_log.thread_id,start_log.task_time," \
              "(end_log.task_time-start_log.task_time) as dur_time " \
              "from FftsLog end_log " \
              "join FftsLog start_log on end_log.thread_id=start_log.thread_id " \
              "and end_log.subtask_id=start_log.subtask_id and end_log.stream_id=start_log.stream_id " \
              "and end_log.task_id=start_log.task_id " \
              "where end_log.task_type='100011' and start_log.task_type='100010' " \
              "and start_log.task_time > 1 and end_log.task_time < 2 " \
              "group by end_log.subtask_id,end_log.thread_id, end_log.task_id " \
              "order by end_log.subtask_id"
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)):
            check = SubTaskCalculator(file_list, self.sample_config)
            ret = check._get_thread_task_time_sql()
            self.assertEqual(ret, sql)

    def test_get_subtask_time_sql(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        sql = "Select end_log.subtask_id, end_log.task_id,end_log.stream_id,end_log.subtask_type," \
              "end_log.ffts_type,start_log.task_time as start_time, (end_log.task_time-start_log.task_time)" \
              " as dur_time from FftsLog end_log join FftsLog start_log " \
              "on end_log.subtask_id=start_log.subtask_id and end_log.stream_id=start_log.stream_id " \
              "and end_log.task_id=start_log.task_id where end_log.task_type='100011' " \
              "and start_log.task_type='100010' and start_log.task_time > 1 " \
              "and end_log.task_time < 2 group by end_log.subtask_id, end_log.task_id order by start_time"
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)):
            check = SubTaskCalculator(file_list, self.sample_config)
            ret = check._get_subtask_time_sql()
            self.assertEqual(ret, sql)

    def test_get_ffts_task_data(self):
        with mock.patch(MODELNAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_ffts_task_data()
        self.assertEqual(ret, [1])

    def test_get_timeline_data(self):
        with mock.patch(MODELNAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_timeline_data()
        self.assertEqual({}, ret)
        thread_data_list = ([0, 1, 2, 3, 4, 5, 6, 7, 8],)
        subtask_data_list = ([5, 6, 7, 8, 9, 1, 2, 4, 6],)
        with mock.patch(MODELNAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(MODELNAMESPACE + '.FftsLogModel._get_thread_time_data', return_value=thread_data_list), \
                mock.patch(MODELNAMESPACE + '.FftsLogModel._get_subtask_time_data', return_value=subtask_data_list):
            check = FftsLogModel('test', 'test', 'test')
            ret = check.get_timeline_data()
        self.assertEqual({'acsq_task_list': [], 'subtask_data_list': ([5, 6, 7, 8, 9, 1, 2, 4, 6],),
                          'thread_data_list': ([0, 1, 2, 3, 4, 5, 6, 7, 8],)}, ret)

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
