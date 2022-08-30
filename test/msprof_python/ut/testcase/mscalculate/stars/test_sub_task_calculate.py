import unittest
from unittest import mock

import pytest

from mscalculate.stars.sub_task_calculate import SubTaskCalculator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.stars.sub_task_calculate'


class TestSubTaskCalculator(unittest.TestCase):
    sample_config = {"result_dir": "test"}

    def test_ms_run(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.SubTaskCalculator.calculate'), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.SubTaskCalculator.save'):
            check = SubTaskCalculator(file_list, self.sample_config)
            check.ms_run()
            check = SubTaskCalculator({}, self.sample_config)
            check.ms_run()

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
        sql = "Select end_log.subtask_id, end_log.task_id,end_log.stream_id,end_log.subtask_type,end_log.ffts_type," \
              "start_log.task_time as start_time, (end_log.task_time-start_log.task_time) as dur_time " \
              "from FftsLog end_log " \
              "join FftsLog start_log on end_log.subtask_id=start_log.subtask_id " \
              "and end_log.stream_id=start_log.stream_id and end_log.task_id=start_log.task_id " \
              "where end_log.task_type='100011' and start_log.task_type='100010' " \
              "and start_log.task_time > 1 and end_log.task_time < 2 " \
              "group by end_log.subtask_id, end_log.task_id " \
              "order by start_time"
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iter_interval', return_value=(1, 2)):
            check = SubTaskCalculator(file_list, self.sample_config)
            ret = check._get_subtask_time_sql()
            self.assertEqual(ret, sql)
