import unittest
from unittest import mock

from constant.ut_db_name_constant import DB_FFTS_PMU
from constant.ut_db_name_constant import TABLE_FFTS_PMU
from model.stars.ffts_pmu_model import FftsPmuModel
from msparser.aic.ffts_pmu_parser import FftsPmuBean
from sqlite.db_manager import DBOpen

NAMESPACE = 'model.stars.ffts_pmu_model'


class TestFftsPmuModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config',
                        return_value={"ai_core_profiling_events": '0x8,0x9,0x1',
                                      "aiv_profiling_events": '0xA,0xB'}), \
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

    def test_insert_task_pmu_data(self):
        data_1 = [1, 2, 3, 4, 5, 6, 7, 89, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 54, 1, 55, 5]
        data_2 = [1, 2, 3, 4, 5, 6, 7, 10000, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 9, 8, 7]
        pmu_data_list = [FftsPmuBean(data_1), FftsPmuBean(data_2)]
        with mock.patch(NAMESPACE + '.FftsPmuModel.insert_data_to_db'), \
             mock.patch(NAMESPACE + '.CalculateAiCoreData.compute_ai_core_data', return_value=(1, {'a': 'b'})), \
             mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config',
                        return_value={"ai_core_profiling_events": "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x5",
                                      "aiv_profiling_events": "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x5"}):
            check = FftsPmuModel('test', 'test', ['test'])
            check.insert_task_pmu_data(pmu_data_list)

    def test_flush(self):
        with mock.patch(NAMESPACE + '.FftsPmuModel.insert_task_pmu_data'):
            check = FftsPmuModel('test', 'test', ['test'])
            check.flush([1])
