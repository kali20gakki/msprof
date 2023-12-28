import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
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

    def test_add_pipe_time_should_return_data_endswith_time_when_data_endswith_ratio_or_ratio_extra(self):
        pmu_dict = {
            'mac_ratio_extra': [0.33], 'scalar_ratio': [0.8], 'mte1_ratio_extra': [0.11], 'mte2_ratio': [0.21],
            'fixpipe_ratio': [10], 'icache_miss_rate': [0.08]
        }
        target_dict = {
            'mac_ratio_extra': [0.33], 'mac_time': [330], 'scalar_ratio': [0.8], 'scalar_time': [800.0],
            'mte1_ratio_extra': [0.11], 'mte1_time': [110.0], 'mte2_ratio': [0.21], 'mte2_time': [210],
            'fixpipe_ratio': [10], 'fixpipe_time': [10000], 'icache_miss_rate': [0.08]
        }
        check = CalculateAiCoreData('')
        result = check.add_pipe_time(pmu_dict, 1000, 'PipeUtilization')
        self.assertEqual(result, target_dict)

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

    def test_cal_addition_when_normal_then_pass(self):
        events_name_list = ["vec_fp16_128lane_ratio", "vec_fp16_64lane_ratio", "mac_fp16_ratio",
                            "mac_int8_ratio", "mte1_iq_full_ratio", "mte2_iq_full_ratio",
                            "mte3_iq_full_ratio", "cube_iq_full_ratio", "vec_iq_full_ratio",
                            "icache_miss_rate", "icache_req_ratio", "fixpipe_ratio", "vec_exe_ratio",
                            "main_mem_read_bw(GB/s)", "main_mem_write_bw(GB/s)", "mte2_ratio", "mte3_ratio",
                            "control_flow_prediction_ratio", "control_flow_mis_prediction_rate"]
        ai_core_profiling_events = {"vec_fp16_128lane_ratio": [1], "vec_fp16_64lane_ratio": [1], "mac_fp16_ratio": [1],
                                    "mac_int8_ratio": [1], "mte1_iq_full_ratio": [1], "mte2_iq_full_ratio": [1],
                                    "mte3_iq_full_ratio": [1], "cube_iq_full_ratio": [1], "vec_iq_full_ratio": [1],
                                    "icache_req_ratio": [1], "icache_miss_rate": [1], "fixpipe_ratio": [0],
                                    "vec_exe_ratio": [0], "main_mem_read_bw(GB/s)": [5], "main_mem_write_bw(GB/s)": [4],
                                    "mte2_ratio": [2], "mte3_ratio": [4],
                                    "control_flow_prediction_ratio": [3], "control_flow_mis_prediction_rate": [2]}
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

    def test_calculate_vec_fp16_ratio_should_return_with_vec_fp16_ratio_when_exist_vec_fp16_data(self):
        events_name_list = ["vec_fp16_128lane_ratio", "vec_fp16_64lane_ratio"]
        ai_core_profiling_events = {"vec_fp16_128lane_ratio": [0.36], "vec_fp16_64lane_ratio": [0.55]}
        target_res = {
            "vec_fp16_128lane_ratio": [0.36], "vec_fp16_64lane_ratio": [0.55],
            "vec_fp16_ratio": [0.91]
        }
        check = CalculateAiCoreData('123')
        res = check._calculate_vec_fp16_ratio(events_name_list, ai_core_profiling_events)
        self.assertEqual(res, target_res)

    def test_calculate_cube_fops_should_return_with_cube_fops_when_exist_mac_ratio_data(self):
        events_name_list = ["mac_fp16_ratio", "mac_int8_ratio"]
        ai_core_profiling_events = {"mac_fp16_ratio": [0.4], "mac_int8_ratio": [0.5]}
        target_res = {
            "mac_fp16_ratio": [0.4], "mac_int8_ratio": [0.5],
            "cube_fops": [11468.8]
        }
        task_cyc = 1
        check = CalculateAiCoreData('123')
        res = check._calculate_cube_fops(events_name_list, ai_core_profiling_events, task_cyc)
        self.assertEqual(res, target_res)

    def test_calculate_mac_ratio_extra_should_return_with_mac_ratio_extra_when_exist_mac_ratio_data(self):
        events_no_vec = ["mac_fp16_ratio", "mac_int8_ratio", "fixpipe_ratio"]
        events_data_no_vec = {
            "mac_fp16_ratio": [0.4], "mac_int8_ratio": [0.5], "cube_fops": [11468.8]
        }
        target_res_no_vec = {"mac_ratio_extra": [0.9]}
        check = CalculateAiCoreData('123')
        res = check._calculate_mac_ratio_extra(events_no_vec, events_data_no_vec)
        self.assertEqual(res, target_res_no_vec)

        events_exist_vec = ["vec_exe_ratio", "mac_fp16_ratio", "mac_int8_ratio", "fixpipe_ratio"]
        events_data_exist_vec = {
            "vec_exe_ratio": [0.3], "mac_fp16_ratio": [0.4],
            "mac_int8_ratio": [0.5], "cube_fops": [11468.8]
        }
        target_res_exist_vec = {"vec_exe_ratio": [0.3], "mac_ratio_extra": [0.9]}
        check = CalculateAiCoreData('123')
        res = check._calculate_mac_ratio_extra(events_exist_vec, events_data_exist_vec)
        self.assertEqual(res, target_res_exist_vec)

        events = ["mac_fp_ratio", "mac_int_ratio"]
        events_data = {"mac_fp_ratio": [0.3], "mac_int_ratio": [0.4]}
        target_res = {"mac_ratio_extra": [0.7]}
        check = CalculateAiCoreData('123')
        res = check._calculate_mac_ratio_extra(events, events_data)
        self.assertEqual(res, target_res)

    def test_calculate_iq_full_ratio_should_return_with_iq_full_ratio_when_exist_iq_full_ratio_data(self):
        events_name_list = [
            "mte1_iq_full_ratio", "mte2_iq_full_ratio", "mte3_iq_full_ratio",
            "cube_iq_full_ratio", "vec_iq_full_ratio"
        ]
        ai_core_profiling_events = {
            "mte1_iq_full_ratio": [0.15], "mte2_iq_full_ratio": [0.1], "mte3_iq_full_ratio": [0.2],
            "cube_iq_full_ratio": [0.1], "vec_iq_full_ratio": [0.1],
        }
        target_res = {
            "mte1_iq_full_ratio": [0.15], "mte2_iq_full_ratio": [0.1], "mte3_iq_full_ratio": [0.2],
            "cube_iq_full_ratio": [0.1], "vec_iq_full_ratio": [0.1], "iq_full_ratio": [0.65]
        }
        check = CalculateAiCoreData('123')
        res = check._calculate_iq_full_ratio(events_name_list, ai_core_profiling_events)
        self.assertEqual(res, target_res)

    def test_calculate_icache_miss_rate_should_return_with_icache_miss_rate_when_exist_icache_data(self):
        events_name_list = ["icache_miss_rate", "icache_req_ratio"]
        ai_core_profiling_events = {"icache_miss_rate": [2000], "icache_req_ratio": [5000]}
        target_res = {"icache_miss_rate": [0.4], "icache_req_ratio": [5000]}
        check = CalculateAiCoreData('123')
        res = check._calculate_icache_miss_rate(events_name_list, ai_core_profiling_events)
        self.assertEqual(res, target_res)

    def test_calculate_memory_bandwidth_should_return_bandwidth_when_exist_bandwidth_data(self):
        events_name_list = [
            "main_mem_read_bw(GB/s)", "main_mem_write_bw(GB/s)",
            "mte2_ratio", "mte3_ratio"
        ]
        ai_core_profiling_events = {
            "main_mem_read_bw(GB/s)": [2000], "main_mem_write_bw(GB/s)": [1500],
            "mte2_ratio": [4], "mte3_ratio": [6]
        }
        target_res = {"main_mem_read_bw(GB/s)": [500], "main_mem_write_bw(GB/s)": [250]}
        check = CalculateAiCoreData('123')
        res = check._calculate_memory_bandwidth(events_name_list, ai_core_profiling_events)
        self.assertEqual(res, target_res)

    def test_calculate_control_flow_mis_prediction_rate_should_return_rate_when_exist_prediction_data(self):
        events_name_list = ["control_flow_prediction_ratio", "control_flow_mis_prediction_rate"]
        ai_core_profiling_events = {
            "control_flow_prediction_ratio": [400], "control_flow_mis_prediction_rate": [300]
        }
        target_res = {"control_flow_prediction_ratio": [400], "control_flow_mis_prediction_rate": [0.75]}
        check = CalculateAiCoreData('123')
        res = check._calculate_control_flow_mis_prediction_rate(events_name_list, ai_core_profiling_events)
        self.assertEqual(res, target_res)

    def test_calculate_ub_read_bw_vector_should_add_vector(self):
        events_name_list = ["ub_read_bw_vector(GB/s)"]
        ChipManager().chip_id = ChipModel.CHIP_V1_1_1
        ai_core_profiling_events = {"ub_read_bw_vector(GB/s)": [2.4, 5.7]}
        check = CalculateAiCoreData('123')
        res = check._calculate_ub_read_bw_vector(events_name_list, ai_core_profiling_events)
        res = res.get("ub_read_bw_vector(GB/s)")[0]
        self.assertLess(abs(res - 8.1), 1e-10)

if __name__ == '__main__':
    unittest.main()
