import unittest
from unittest import mock

from common_func.platform.chip_manager import ChipManager
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from common_func.info_conf_reader import InfoConfReader
from profiling_bean.prof_enum.chip_model import ChipModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}
NAMESPACE = 'mscalculate.calculate_ai_core_data'


class TestCalculateAiCoreData(unittest.TestCase):
    def test_add_fops_header(self):
        metric_key = "ai_core_metrics"
        metrics = []
        with mock.patch('common_func.config_mgr.ConfigMgr.read_sample_config',
                        return_value={"ai_core_metrics": "ArithmeticUtilization"}):
            key = CalculateAiCoreData('123')
            key.add_fops_header(metric_key, metrics)

    def test_add_pipe_time_for(self):
        key = CalculateAiCoreData('')
        pmu_dict = {'vec_ratio': [0], 'mac_ratio': [0], 'scalar_ratio': [0], 'mte1_ratio': [0], 'mte2_ratio': [0],
                    'mte3_ratio': [0], 'icache_req_ratio': [0], 'icache_miss_rate': [0]}
        result = key.add_pipe_time(pmu_dict, 0, 'PipeUtilization')
        self.assertEqual(len(result.values()), 14)

    def test_add_pipe_time_if_not(self):
        pmu_dict = {'vec_ratio': [0], 'mac_ratio': [0.8], 'scalar_ratio': [0.11], 'mte1_ratio': [0], 'mte2_ratio': [0],
                    'mte3_ratio': [0.08], 'icache_req_ratio': [0], 'icache_miss_rate': [0]}
        key = CalculateAiCoreData('')
        result = key.add_pipe_time(pmu_dict, 1000, 'PipeUtilization')
        self.assertEqual(len(result.values()), 14)

    def test_update_fops_data_1(self):
        field = "vector_fops"
        algo = "r4b_num, r4e_num, r4f_num"
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        key = CalculateAiCoreData('123')
        result = key.update_fops_data(field, algo)
        self.assertEqual(result, "32.0, 64.0, 32.0")
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        key = CalculateAiCoreData('123')
        result = key.update_fops_data(field, algo)
        self.assertEqual(result, "64.0, 16.0, 16.0")
        ChipManager().chip_id = ChipModel.CHIP_V2_1_0
        key = CalculateAiCoreData('123')
        result = key.update_fops_data(field, algo)
        self.assertEqual(result, "64.0, 64.0, 32.0")

    def test_update_fops_data_2(self):
        field = "vector"
        algo = "r4b_num, r4e_num, r4f_num"
        key = CalculateAiCoreData('123')
        result = key.update_fops_data(field, algo)
        self.assertEqual(result, "r4b_num, r4e_num, r4f_num")

    def test_compute_ai_core_data(self):
        events_name_list = ['vec_ratio', 'ub_read_bw(GB/s)', 'abc']
        ai_core_profiling_events = {"bw": []}
        task_cyc = 26194
        pmu_data = [26194, 0, 1421]
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': '1150'}]}
        with mock.patch(NAMESPACE + '.CalculateAiCoreData._CalculateAiCoreData__cal_addition',
                        return_value={'abc': [1421],
                                      'bw': [], 'ub_read_bw(GB/s)': [0.0],
                                      'vec_ratio': [1.0]}):
            key = CalculateAiCoreData('123')
            result = key.compute_ai_core_data(events_name_list, ai_core_profiling_events, task_cyc, pmu_data)
        self.assertEqual(result, (['vec_ratio', 'ub_read_bw(GB/s)', 'abc'], {'abc': [1421],
                                                                             'bw': [], 'ub_read_bw(GB/s)': [0.0],
                                                                             'vec_ratio': [1.0]}))
        with mock.patch(NAMESPACE + '.CalculateAiCoreData._cal_pmu_metrics',
                        side_effect=OSError), \
                mock.patch(NAMESPACE + '.CalculateAiCoreData._CalculateAiCoreData__cal_addition'), \
                mock.patch(NAMESPACE + '.logging.error'):
            key = CalculateAiCoreData('123')
            check = (['vec_ratio', 'ub_read_bw(GB/s)', 'abc'],
                     {'bw': [], 'vec_ratio': [1.0], 'ub_read_bw(GB/s)': [0.0], 'abc': [1421]})
            result = key.compute_ai_core_data(events_name_list, ai_core_profiling_events, task_cyc, pmu_data)
        self.assertEqual(result, check)

    def test_cal_addition(self):
        events_name_list = ["vec_fp16_128lane_ratio", "vec_fp16_64lane_ratio", "mac_fp16_ratio",
                            "mac_int8_ratio", "mte1_iq_full_ratio", "mte2_iq_full_ratio",
                            "mte3_iq_full_ratio", "cube_iq_full_ratio", "vec_iq_full_ratio",
                            "icache_miss_rate", "icache_req_ratio"]
        ai_core_profiling_events = {"vec_fp16_128lane_ratio": [1], "vec_fp16_64lane_ratio": [1], "mac_fp16_ratio": [1],
                                    "mac_int8_ratio": [1], "mte1_iq_full_ratio": [1], "mte2_iq_full_ratio": [1],
                                    "mte3_iq_full_ratio": [1], "cube_iq_full_ratio": [1], "vec_iq_full_ratio": [1],
                                    "icache_req_ratio": [1], "icache_miss_rate": [1]}
        task_cyc = 1
        with mock.patch(NAMESPACE + '.CalculateAiCoreData.add_vector_data'):
            key = CalculateAiCoreData('123')
            key._CalculateAiCoreData__cal_addition(events_name_list,
                                                   ai_core_profiling_events, task_cyc)

    def test_get_vector_num(self):
        InfoConfReader()._info_json = {"platform_version": "3"}
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        key = CalculateAiCoreData('123')
        result = key.get_vector_num()

        self.assertEqual(result, [64.0, 16.0, 16.0])

        ChipManager().chip_id = ChipModel.CHIP_V2_1_0
        key = CalculateAiCoreData('123')
        result = key.get_vector_num()
        self.assertEqual(result, [64.0, 64.0, 32.0])

    def test_add_vector_data(self):
        events_name_list = ["vec_fp16_128lane_ratio", "vec_fp16_64lane_ratio", "vec_fp32_ratio",
                            "vec_int32_ratio", "vec_misc_ratio"]
        ai_core_profiling_events = {"vec_fp16_128lane_ratio": [1], "vec_fp16_64lane_ratio": [1],
                                    "vec_fp32_ratio": [1], "vec_int32_ratio": [1], "vec_misc_ratio": [1]}
        task_cyc = 1
        with mock.patch(NAMESPACE + '.CalculateAiCoreData.get_vector_num', return_value=[1, 1, 1]):
            key = CalculateAiCoreData('123')
            key.add_vector_data(events_name_list, ai_core_profiling_events, task_cyc)


if __name__ == '__main__':
    unittest.main()
