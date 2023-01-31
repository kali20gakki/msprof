import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from analyzer.data_analysis_factory import DataAnalysisFactory
from constant.constant import CONFIG, INFO_JSON

NAMESPACE = 'analyzer.data_analysis_factory'


class TestDataAnalysisFactory(unittest.TestCase):

    def test_parse_data(self):
        check = DataAnalysisFactory(CONFIG)
        result = check.parse_data()
        self.assertEqual(result, None)
        with mock.patch('analyzer.scene_base.op_summary_op_scene.OpSummaryOpScene.run'), \
                mock.patch('analyzer.scene_base.op_counter_op_scene.OpCounterOpScene.run'):
            check = DataAnalysisFactory(CONFIG)
            check.scene = 'single_op'
            check.parse_data()

    def test_run(self):
        with mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                        return_value="step_info"),\
                mock.patch(NAMESPACE + '.DataAnalysisFactory.parse_data'):
            check = DataAnalysisFactory(CONFIG)
            ProfilingScene().init('')
            check.run()
