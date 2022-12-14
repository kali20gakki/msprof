import unittest
from unittest import mock
import copy

from common_func.msprof_iteration import MsprofIteration
from common_func.batch_counter import BatchCounter
from common_func.constant import Constant
from analyzer.scene_base.profiling_scene import ProfilingScene
from mscalculate.hwts.acsq_calculator import AcsqCalculator

from mock_tools import ClassMock


NAMESPACE = 'msinterface.msprof_export_data'


class TestAcsqCalculator(unittest.TestCase):

    def test_add_batch_id(self: any) -> None:
        expect_res = [(1, 2, 3, 4, 5, 6, 7, 1)]
        test_func = getattr(AcsqCalculator, "_add_batch_id")

        test_object = mock.Mock()
        setattr(test_object, "_sample_config", {"model_id": 1, "iter_id": 2})
        setattr(test_object, "_project_path", "")
        test_object.PREP_DATA_LENGTH = 7

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
                batch_counter = BatchCounter()
                batch_counter.calculate_batch.return_value = 1
                res = test_func(test_object, [(1, 2, 3, 4, 5, 6, 7)])

        profiling_scene.is_operator = is_operator_func
        self.assertEqual(expect_res, res)
