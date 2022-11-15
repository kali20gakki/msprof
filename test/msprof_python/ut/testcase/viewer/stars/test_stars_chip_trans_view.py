import json
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from viewer.stars.stars_chip_trans_view import StarsChipTransView

NAMESPACE = 'viewer.stars.stars_chip_trans_view'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestStarsChipTransView(unittest.TestCase):

    def test_get_timeline_data(self):
        InfoJsonReaderManager(InfoJson(pid=123)).process()
        with mock.patch(NAMESPACE + '.ViewModel.init', return_value=True), \
             mock.patch(NAMESPACE + '.ViewModel.check_table', return_value=True), \
             mock.patch(NAMESPACE + '.ViewModel.get_sql_data', return_value=[[0, 1, 2, 3]]):
            res = StarsChipTransView(CONFIG, CONFIG).get_timeline_data()
        self.assertEqual(len(json.loads(res)), 7)

