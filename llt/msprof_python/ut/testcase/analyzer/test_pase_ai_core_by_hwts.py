import struct
import unittest
from collections import OrderedDict
from unittest import mock
import pytest
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from constant.constant import CONFIG, INFO_JSON
from analyzer.parse_ai_core_by_hwts import ParseAiVectorCoreByHwts, BaseParseDataByHwts
from sqlite.db_manager import DBManager

NAMESPACE = 'analyzer.parse_ai_core_by_hwts'


class TestParseAiVectorCoreByHwts(unittest.TestCase):
    pmu_event = {'aiv_profiling_events': "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x55"}

    def test_move_total_forward(self):
        with mock.patch(NAMESPACE + '.logging.info'):
            check = BaseParseDataByHwts('aiVectorCore.data.0.slice_0',
                                        self.pmu_event, None, 960000000)
            check.ai_core_profiling_events = OrderedDict([('vec_ratio', [0.04])])
            result = check._BaseParseDataByHwts__move_total_forward()
        self.assertEqual(result, None)

    def test_read_binary_data(self):
        data = struct.pack("=BBHHHIIqqqqqqqqqqIIIIIIII",
                           5, 2, 27603, 0, 1, 0, 0, 3199839, -1, 151274, 0, 37793, 0, 32584,
                           3192994, 19002, 26, 2, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch(NAMESPACE + '.logging.info'), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = ParseAiVectorCoreByHwts('aiVectorCore.data.0.slice_0',
                                            self.pmu_event, None, 960000000)
            check.read_binary_data()
        with mock.patch(NAMESPACE + '.logging.info'), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch('os.path.exists', return_value=True), \
             pytest.raises(ProfException) as error:
            check = ParseAiVectorCoreByHwts('aiVectorCore', self.pmu_event,
                                            None, 960000000)
            check.read_binary_data()
        self.assertEqual(error.value.code, 10)

    def test_get_current_iter_id(self):
        create_sql = "create table if not exists test(iter_id int, task_id int)"
        db_manager = DBManager()
        res = db_manager.create_table('rumtime.db')
        res[1].execute(create_sql)
        check = BaseParseDataByHwts('aiVectorCore.data.0.slice_0',
                                        self.pmu_event, res[0], 960000000)
        check.cur = res[1]
        check._BaseParseDataByHwts__get_current_iter_id('test')
        db_manager.destroy(res)

    def test_save_db(self):
        ai_core_profiling_events = OrderedDict(
            [('vec_ratio', [31, 4, 87, 88, 89, 90, 91, 92, 93, 94, 95]),
             ('icache_req_ratio', [1]), ('vec_fp16_128lane_ratio', [2]),
             ('vec_fp16_64lane_ratio', [2]),
             ('mac_ratio', [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
             ('scalar_ratio', [7, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
             ('mte1_ratio', [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
             ('mte2_ratio', [3, 1, 8, 9, 10, 11, 12, 13, 14, 15, 16]),
             ('mte3_ratio', [74, 99, 22, 23, 24, 25, 26, 27, 28, 29, 30]),
             ('icache_miss_rate', [0.1, 810, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1])])
        aicore_data = {'ai_data': [
            ['', '', 4753319, '', '0', -1, ''], ['0', '0000', 3199839, -1, '0', 1, 2],
            ['0', '0001', 172583, -1, '0', 3, 2], ['0', '0010', 172530, -1, '0', 5, 2],
            ['0', '0011', 172932, -1, '0', 6, 2], ['0', '0100', 172265, -1, '0', 8, 2],
            ['0', '0101', 173006, -1, '0', 10, 2], ['0', '0110', 172318, -1, '0', 11, 2],
            ['0', '0111', 172615, -1, '0', 13, 2], ['0', '1000', 172575, -1, '0', 15, 2],
            ['0', '1001', 172656, -1, '0', 17, 2]]}
        db_manager = DBManager()
        res = db_manager.create_table('runtime.db')
        with mock.patch(NAMESPACE + '.insert_metric_value', return_value=False), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = ParseAiVectorCoreByHwts('aiVectorCore.data.0.slice_0',
                                            self.pmu_event, res[0], 960000000)
            check.ai_core_profiling_events = ai_core_profiling_events
            result = check.save_db('AivMetricSummary')
            self.assertEqual(result, None)
        check = ParseAiVectorCoreByHwts('aiVectorCore.data.0.slice_0',
                                        self.pmu_event, res[0], 960000000)
        check.ai_core_profiling_events = ai_core_profiling_events
        check.aicore_data["ai_data"] = aicore_data
        result = check.save_db('AivMetricSummary')
        self.assertEqual(result, None)
        res[1].execute("drop table AivMetricSummary")
        res[1].execute("CREATE TABLE IF NOT EXISTS AivMetricSummary(total_cycles numeric,"
                       "vec_ratio numeric,mac_ratio numeric,scalar_ratio numeric,mte1_ratio numeric,"
                       "mte2_ratio numeric,mte3_ratio numeric,icache_miss_rate numeric, device_id INT,"
                       " task_id INT, stream_id INT,index_id INT)")
        check = ParseAiVectorCoreByHwts('aiVectorCore.data.0.slice_0',
                                        self.pmu_event, res[0], 1150000000)
        check.ai_core_profiling_events = ai_core_profiling_events
        check.aicore_data["ai_data"] = aicore_data
        result = check.save_db('AivMetricSummary')
        self.assertEqual(result, None)
        res[1].execute("drop table AivMetricSummary")
        db_manager.destroy(res)

    def test_process(self):
        with mock.patch(NAMESPACE + '.ParseAiVectorCoreByHwts._parse_ai_vector_core_pmu_event'), \
             mock.patch(NAMESPACE + '.ParseAiVectorCoreByHwts.read_binary_data'), \
             mock.patch(NAMESPACE + '.ParseAiVectorCoreByHwts.save_db'):
            ParseAiVectorCoreByHwts('aiVectorCore.data.0.slice_0', CONFIG, None, 960000000).process()

    def test_parse_ai_vector_core_pmu_event(self):
        check = ParseAiVectorCoreByHwts('aiVectorCore.data.0.slice_0',
                                        {'aiv_profiling_events': "0x8,0xa"}, None, 960000000)
        check._parse_ai_vector_core_pmu_event()
        self.assertEqual(check.events_name_list, ['vec_ratio', 'mac_ratio'])
        with mock.patch(NAMESPACE + '.read_cpu_cfg', return_value={}):
            check._parse_ai_vector_core_pmu_event()
            self.assertEqual(check.events_name_list, ['0x8', '0xa'])
