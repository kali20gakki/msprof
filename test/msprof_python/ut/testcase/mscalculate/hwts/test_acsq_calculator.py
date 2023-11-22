import unittest
from unittest import mock

from common_func.batch_counter import BatchCounter
from common_func.profiling_scene import ProfilingScene
from mock_tools import ClassMock
from mscalculate.hwts.acsq_calculator import AcsqCalculator
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.hwts.acsq_calculator'


class TestAcsqCalculator(unittest.TestCase):
    file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0']}
    sample_config = {"result_dir": ""}

    def test_ms_run_when_no_table_exist_then_create_table_and_calculate(self):
        ProfilingScene()._scene = "single_op"
        with mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=False), \
                mock.patch(NAMESPACE + '.AcsqCalculator._get_data_from_task_table'), \
                mock.patch('msmodel.step_trace.ts_track_model.TsTrackModel.get_step_end_list_with_iter_range',
                           return_value=[]):
            check = AcsqCalculator(self.file_list, self.sample_config)
            check._log_data = [
                (13, 0, 16764098231950, 16764098231970, 'PLACE_HOLDER_SQE'),
                (39716, 2, 16764420628930, 16764422893830, 'AI_CORE')
            ]
            check._iter_range = IterationRange(model_id=1,
                                               iteration_id=1,
                                               iteration_count=1)
            check.ms_run()

    def test_ms_run_when_table_exist_then_do_not_execute(self):
        ProfilingScene().set_all_export(True)
        with mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = AcsqCalculator(self.file_list, self.sample_config)
            check.calculate = mock.Mock()
            check.save = mock.Mock()
            check.ms_run()
            check.calculate.assert_not_called()
            check.save.assert_not_called()

    def test_get_data_from_task_table(self):
        AcsqCalculator(self.file_list, self.sample_config)._get_data_from_task_table()

        with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            AcsqCalculator(self.file_list, self.sample_config)._get_data_from_task_table()
