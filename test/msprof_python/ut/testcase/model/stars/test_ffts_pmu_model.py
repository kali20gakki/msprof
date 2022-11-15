import unittest
from unittest import mock

from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from constant.ut_db_name_constant import DB_FFTS_PMU
from constant.ut_db_name_constant import TABLE_FFTS_PMU
from msmodel.stars.ffts_pmu_model import FftsPmuManager
from msmodel.stars.ffts_pmu_model import FftsPmuModel
from profiling_bean.stars.ffts_plus_pmu import FftsPlusPmuBean
from profiling_bean.stars.ffts_pmu import FftsPmuBean
from sqlite.db_manager import DBOpen

NAMESPACE = 'msmodel.stars.ffts_pmu_model'


class TestFftsPmuModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config',
                        return_value={"ai_core_metrics": "PipeUtilization",
                                      "aiv_metrics": "PipeUtilization",
                                      "ai_core_profiling_events": '0x8,0x9,0x1',
                                      "aiv_profiling_events": '0xA,0xB'}), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.FftsPmuModel._insert_metric_value'):
            check = FftsPmuModel('test', 'test', ['test'])
            check.create_table()

    def test_insert_metric_value(self):
        with DBOpen(DB_FFTS_PMU) as db_open, \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config',
                           return_value={"ai_core_metrics": "PipeUtilization",
                                         "aiv_metrics": "PipeUtilization",
                                         "ai_core_profiling_events": '0x8,0x9,0x1',
                                         "aiv_profiling_events": '0xA,0xB'}):
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
        with mock.patch(NAMESPACE + '.FftsPmuModel.insert_data_to_db'), \
                mock.patch(NAMESPACE + '.CalculateAiCoreData.compute_ai_core_data', return_value=(1, {'a': 'b'})), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config',
                           return_value={"ai_core_metrics": "PipeUtilization",
                                         "aiv_metrics": "PipeUtilization",
                                         "ai_core_profiling_events": "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x5",
                                         "aiv_profiling_events": "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x5"}):
            check = FftsPmuModel('test', 'test', ['test'])
            check.profiling_events = {
                'aic': ['total_time(ms)', 'total_cycles', 'vec_ratio', 'mac_ratio', 'scalar_ratio', 'mte1_ratio',
                        'mte2_ratio', 'mte3_ratio', 'icache_miss_rate'], 'aiv': []}
            check.update_pmu_list(pmu_data_list)

    def test_flush(self):
        data = b'\00' * 128
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config',
                        return_value={"ai_core_metrics": "PipeUtilization",
                                      "aiv_metrics": "PipeUtilization",
                                      "ai_core_profiling_events": "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x5",
                                      "aiv_profiling_events": "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x5"}), \
                mock.patch(NAMESPACE + '.FftsPmuModel.insert_data_to_db'):
            check = FftsPmuModel('test', 'test', ['test'])
            InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[
                DeviceInfo(hwts_frequency='50', aic_frequency='1000', aiv_frequency='1800').device_info])).process()
            check.flush({'block_task': [FftsPlusPmuBean.decode(data)],
                         'origin_task': [FftsPmuBean.decode(data)]})
            check.flush({})


class TestFftsPmuManager(unittest.TestCase):

    def test_calculate_block_pmu_list(self):
        data_1 = [1, 2, 3, 4, 5, 6, 7, 89, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 54, 1, 55, 5]
        data_2 = [1, 2, 3, 4, 5, 6, 7, 32768, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 9, 8, 7]
        pmu_data_list = [FftsPmuBean(data_1), FftsPmuBean(data_2)]
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events',
                           return_value=['vec_ratio', 'mac_ratio', 'scalar_ratio',
                                         'mte1_ratio', 'mte2_ratio', 'mte3_ratio',
                                         'icache_req_ratio', 'icache_miss_rate']), \
                mock.patch(NAMESPACE + '.FftsPmuManager._get_pmu_value'):
            check = FftsPmuManager('test')
            check.calculate_block_pmu_list(pmu_data_list[1])
            check.calculate_block_pmu_list(pmu_data_list[0])

    def test_add_block_pmu_list(self):
        block_dict = {43 - 13 - 0: [['MIX_AIC', (349456, (1161, 0, 1088, 0, 7809, 7380, 391, 13))],
                                    ['MIX_AIC', (349425, (1161, 0, 1121, 0, 7915, 7409, 407, 13))]],
                      43 - 14 - 0: [['MIX_AIV', (349456, (1161, 0, 1088, 0, 7809, 7380, 391, 13))]],
                      43 - 14 - 0: [['MIX_OTHER', (349456, (1161, 0, 1088, 0, 7809, 7380, 391, 13))]]}
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events',
                           return_value=['vec_ratio', 'mac_ratio', 'scalar_ratio',
                                         'mte1_ratio', 'mte2_ratio', 'mte3_ratio',
                                         'icache_req_ratio', 'icache_miss_rate']), \
                mock.patch(NAMESPACE + '.FftsPmuManager._get_pmu_value'):
            check = FftsPmuManager('test')
            check.block_dict = block_dict
            check.add_block_pmu_list()
