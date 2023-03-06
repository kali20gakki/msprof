import unittest
from unittest import mock

from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from constant.ut_db_name_constant import DB_FFTS_PMU
from constant.ut_db_name_constant import TABLE_FFTS_PMU
from msmodel.stars.ffts_pmu_model import FftsPmuModel
from profiling_bean.stars.ffts_block_pmu import FftsBlockPmuBean
from profiling_bean.stars.ffts_pmu import FftsPmuBean
from sqlite.db_manager import DBOpen

NAMESPACE = 'msmodel.stars.ffts_pmu_model'


class TestFftsPmuModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                        return_value=
                        ['total_time(ms)', 'total_cycles', 'vec_time', 'vec_ratio', 'mac_time', 'mac_ratio',
                         'scalar_time', 'scalar_ratio', 'mte1_time', 'mte1_ratio', 'mte2_time', 'mte2_ratio',
                         'mte3_time', 'mte3_ratio', 'icache_miss_rate']), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.FftsPmuModel._insert_metric_value'):
            check = FftsPmuModel('test', 'test', ['test'])
        check.create_table()

    def test_insert_metric_value(self):
        with DBOpen(DB_FFTS_PMU) as db_open:
            metrics = ['(ms)test', '(ms)abc']
            check = FftsPmuModel('test', 'test', ['test'])
            check.conn, check.cur = db_open.db_conn, db_open.db_curs
            check._insert_metric_value(metrics, TABLE_FFTS_PMU)
            with mock.patch(NAMESPACE + '.logging.error'):
                check._insert_metric_value(metrics, TABLE_FFTS_PMU)

    def test_update_pmu_list(self):
        data_1 = [1, 2, 3, 4, 5, 6, 7, 89, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 54, 1, 55, 5]
        data_2 = [1, 2, 3, 4, 5, 6, 7, 10000, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 9, 8, 7]
        pmu_data_list = [FftsPmuBean(data_1), FftsPmuBean(data_2)]
        check = FftsPmuModel('test', 'test', ['test'])
        check.profiling_events = {
            'aic': ['total_time(ms)', 'total_cycles', 'vec_ratio', 'mac_ratio', 'scalar_ratio', 'mte1_ratio',
                    'mte2_ratio', 'mte3_ratio', 'icache_miss_rate'], 'aiv': []}
        check.update_pmu_list(pmu_data_list)

    def test_flush(self):
        data = b'\00' * 128
        with mock.patch(NAMESPACE + '.FftsPmuModel.insert_data_to_db'):
            check = FftsPmuModel('test', 'test', ['test'])
            InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[
                DeviceInfo(hwts_frequency='50', aic_frequency='1000', aiv_frequency='1800').device_info])).process()
            check.flush([])
