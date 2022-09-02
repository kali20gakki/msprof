import unittest
from unittest import mock

from mscalculate.summary.op_summary_calculator import OpSummaryCalculator

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}
NAMESPACE = 'mscalculate.summary.op_summary_calculator'


class TestOpSummaryCalculator(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.OpSummaryModel.create_table'):
            OpSummaryCalculator({}, sample_config=sample_config).ms_run()

    def test_save(self):
        with mock.patch(NAMESPACE + '.OpSummaryModel.create_table'):
            OpSummaryCalculator({}, sample_config=sample_config).save()
