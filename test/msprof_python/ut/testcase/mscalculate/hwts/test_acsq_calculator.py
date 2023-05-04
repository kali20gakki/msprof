import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.batch_counter import BatchCounter
from mock_tools import ClassMock
from mscalculate.hwts.acsq_calculator import AcsqCalculator
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.hwts.acsq_calculator'


class TestAcsqCalculator(unittest.TestCase):
    file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0']}
    sample_config = {"result_dir": ""}

    def test_add_batch_id(self: any) -> None:
        expect_res = [(1, 2, 3, 4, 5, 6, 7, 1)]
        test_func = getattr(AcsqCalculator, "_add_batch_id")

        test_object = mock.Mock()
        setattr(test_object, "_sample_config", {"model_id": 1, "iter_id": 2})
        setattr(test_object, "_project_path", "")
        test_object.PREP_DATA_LENGTH = 7
        test_object._aicpu_collector.aicpu_list = []

        profiling_scene = ProfilingScene()
        is_operator_func = profiling_scene.is_operator
        profiling_scene.is_operator = mock.Mock()
        profiling_scene.is_operator.return_value = False

        msprof_iteration = mock.Mock()
        msprof_iteration.get_parallel_iter_range = mock.Mock()
        msprof_iteration.get_parallel_iter_range.return_value = [1, 2]

        with mock.patch("common_func.msprof_iteration.MsprofIteration.get_parallel_iter_range", return_value=[1, 2]):
            with ClassMock(BatchCounter, mock.Mock()):
                ProfilingScene().init("")
                batch_counter = BatchCounter('test')
                batch_counter.calculate_batch.return_value = 1
                res = test_func(test_object, [(1, 2, 3, 4, 5, 6, 7)])

        profiling_scene.is_operator = is_operator_func
        self.assertEqual(expect_res, res)

    def test_ms_run(self):
        ProfilingScene()._scene = "single_op"
        with mock.patch(NAMESPACE + '.AcsqCalculator._get_data_from_task_table'), \
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

    def test_get_data_from_task_table(self):
        with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', retunr_value='test'), \
                mock.patch('mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector.BatchCounter.init'), \
                mock.patch('mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector.AICpuFromTsCollector'), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                           return_value={}), \
                mock.patch(NAMESPACE + '.logging.warning'):
            AcsqCalculator(self.file_list, self.sample_config)._get_data_from_task_table()
        with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', retunr_value='test'), \
                mock.patch('mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector.BatchCounter.init'), \
                mock.patch('mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector.AICpuFromTsCollector'), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                           return_value={}), \
                mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.init', retunr_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            AcsqCalculator(self.file_list, self.sample_config)._get_data_from_task_table()

    def test_filter_aicpu(self):
        ProfilingScene()._scene = "single_op"
        prep_data = [
            (13, 0, 16764098231950, 16764098231970, 'PLACE_HOLDER_SQE', 1, 4294967295),
            (39716, 2, 16764420628930, 16764422893830, 'AI_CPU', 1, 4294967295)
        ]
        with mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                        return_value={}):
            AcsqCalculator(self.file_list, self.sample_config)._add_batch_id(prep_data)
