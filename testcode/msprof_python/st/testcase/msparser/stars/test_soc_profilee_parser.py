import unittest

from msparser.stars.soc_profiler_parser import SocProfilerParser

NAMESPACE = 'msparser.stars.acc_pmu_parser'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestAccPmuParser(unittest.TestCase):

    def test_init(self):
        SocProfilerParser({}, sample_config)

