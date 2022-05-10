import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from model.aic.aic_pmu_model import AicPmuModel

NAMESPACE = 'model.aic.aic_pmu_model'


class TestPcieModel(unittest.TestCase):

    def test_init(self):
        with mock.patch('model.interface.base_model.BaseModel.init'),\
                mock.patch(NAMESPACE + '.AicPmuModel.create_table'):
            check = AicPmuModel('test')
            check.init()

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.AicPmuModel.clear'),\
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           return_value={'ai_core_profiling_events': '0x64,0x65,0x66'}), \
                mock.patch(NAMESPACE + '.insert_metric_value'):
            check = AicPmuModel('test')
            check.create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AicPmuModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = AicPmuModel('test')
            check.flush([])

    def test_clear(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = AicPmuModel('test')
            check.clear()

