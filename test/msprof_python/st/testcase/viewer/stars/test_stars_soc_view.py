import json
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from viewer.stars.stars_soc_view import StarsSocView

NAMESPACE = 'viewer.stars.stars_soc_view'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestStarsSocView(unittest.TestCase):

    def test_get_timeline_data(self):
        InfoConfReader()._info_json = {"pid": 123}
        with mock.patch(NAMESPACE + '.ViewModel.init', return_value=True), \
             mock.patch(NAMESPACE + '.ViewModel.check_table', return_value=True), \
             mock.patch(NAMESPACE + '.ViewModel.get_sql_data', return_value=[[0, 1, 2, 3]]):
            res = StarsSocView(CONFIG, CONFIG).get_timeline_data()
        self.assertEqual(len(json.loads(res)), 3)
