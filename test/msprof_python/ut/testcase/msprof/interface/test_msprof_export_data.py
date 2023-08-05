import configparser
import json
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from msinterface.msprof_export_data import MsProfExportDataUtils
from msinterface.msprof_timeline import MsprofTimeline
from profiling_bean.prof_enum.chip_model import ChipModel
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from viewer.stars.stars_soc_view import StarsSocView

NAMESPACE = 'msinterface.msprof_export_data'


class TestMsProfExportDataUtils(unittest.TestCase):

    def tearDown(self) -> None:
        MsprofTimeline()._export_data_list = []

    def test_export_data_1(self):
        params = {"data_type": None}
        key = MsProfExportDataUtils()
        result = key.export_data(params)
        self.assertEqual(result, '{"status": 1, "info": "Parameter data_type is none."}')

    def test_export_data_3(self):
        params = {'data_type': 'step_trace', 'project': 't', 'device_id': '0',
                  'job_id': 'job_default', 'export_type': 'timeline', 'iter_id': 1,
                  'export_format': None, 'model_id': 1}
        with mock.patch(NAMESPACE + '.MsProfExportDataUtils._load_export_data_config'), \
             mock.patch(NAMESPACE + '.MsProfExportDataUtils._get_configs_with_data_type',
                        return_value={"handler": '_get_runtime_api_data'}), \
             mock.patch(NAMESPACE + '.MsProfExportDataUtils.add_timeline_data'):
            key = MsProfExportDataUtils()
            result = key.export_data(params)
        self.assertEqual(result, json.dumps({'status': 1, 'info': 'Failed to connect runtime.db'}))

    def test_export_data_4(self):
        params = {"data_type": 123, "export_type": "456"}
        with mock.patch(NAMESPACE + '.MsProfExportDataUtils._load_export_data_config'), \
             mock.patch(NAMESPACE + '.MsProfExportDataUtils._get_configs_with_data_type',
                        return_value={"handler": None}), \
             mock.patch(NAMESPACE + '.MsProfExportDataUtils.add_timeline_data'):
            key = MsProfExportDataUtils()
            result = key.export_data(params)
        self.assertEqual(result, '{"status": 1, "info": "Unable to handler data type 123."}')

    def test_export_data_should_return_error_message_when_input_invalid_timeline(self):
        params = \
        {
            'data_type': 'step_trace', 'project': 't', 'device_id': '0',
            'job_id': 'job_default', 'export_type': 'timeline', 'iter_id': 1,
            'export_format': None, 'model_id': 1
        }
        with mock.patch(NAMESPACE + '.MsProfExportDataUtils._load_export_data_config'), \
                mock.patch(NAMESPACE + '.MsProfExportDataUtils._get_configs_with_data_type',
                           return_value={"handler": '_get_runtime_api_data'}), \
                mock.patch(NAMESPACE + '.MsProfExportDataUtils._get_runtime_api_data', return_value="invalid_data"):
            key = MsProfExportDataUtils()
            expected = ('{"status": 2, "info": "Unable to get step_trace data. Maybe the data is not collected, '
                        'or the data may fail to be analyzed."}')
            self.assertEqual(expected, key.export_data(params))

    def test_add_timeline_data(self):
        params = {"data_type": '123'}
        data = 123
        key = MsProfExportDataUtils()
        key.add_timeline_data(params, data)

    def test_get_configs_with_data_type(self):
        with mock.patch('configparser.ConfigParser.has_option', return_value=True), \
             mock.patch('configparser.ConfigParser.get', return_value='12,34'):
            data_type = {"handler": 123}
            key = MsProfExportDataUtils()
            MsProfExportDataUtils.cfg_parser = configparser.ConfigParser(interpolation=None)
            key._get_configs_with_data_type(data_type)

    def test_get_runtime_api_data_1(self):
        configs = {"db": 'runtime', "table": 'trace'}
        params = {"export_type": "timeline", "project": '123'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='runtime.db'):
            key = MsProfExportDataUtils()
            result = key._get_runtime_api_data(configs, params)
        self.assertEqual(result, '{"status": 1, "info": "Failed to connect runtime.db"}')

    def test_get_runtime_api_data_2(self):
        configs = {"db": '123', "table": '456'}
        params = {"export_type": "export", "project": '123'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='runtime.db'):
            key = MsProfExportDataUtils()
            result = key._get_runtime_api_data(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_task_time_data_1(self):
        configs = {"db": 'runtime.db', "table": '123'}
        params = {"export_type": "timeline", "job_id": '1', "device_id": '1',
                  "iter_id": '1', "project": '1'}
        InfoConfReader()._info_json = {"devices": 123}
        key = MsProfExportDataUtils()
        result = key._get_task_time_data(configs, params)
        self.assertEqual(result, "")

    def test_get_task_time_data_2(self):
        configs = {"db": 'runtime.db', "table": '123'}
        params = {"export_type": "summary", "job_id": '1', "device_id": '1',
                  "iter_id": '1', "project": '1'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            InfoConfReader()._info_json = {"devices": 123}
            key = MsProfExportDataUtils()
            result = key._get_task_time_data(configs, params)
        self.assertEqual(result, (None, [], 0))

    def test_get_task_time_data_3(self):
        configs = {"db": 'trace.db', "table": '123'}
        params = {"export_type": "export", "job_id": '1', "device_id": '1',
                  "iter_id": '1', "project": '1'}
        with mock.patch(NAMESPACE + '.TaskOpViewer.get_task_op_summary', return_value=123):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            InfoConfReader()._info_json = {"devices": 123}
            key = MsProfExportDataUtils()
            result = key._get_task_time_data(configs, params)
        self.assertEqual(result[0], None)

    def test_get_ddr_data_1(self):
        configs = {"db": 'runtime'}
        params = {"export_type": "timeline", "project": '123', "device_id": '456'}
        with mock.patch(NAMESPACE + '.get_ddr_timeline', return_value=111):
            key = MsProfExportDataUtils()
            result = key._get_ddr_data(configs, params)
        self.assertEqual(result, 111)

    def test_get_ddr_data_2(self):
        configs = {"db": 'trace.db'}
        params = {"export_type": "summary", "project": '123', "device_id": '456'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='111'), \
             mock.patch(NAMESPACE + '.get_ddr_data', return_value=222):
            key = MsProfExportDataUtils()
            result = key._get_ddr_data(configs, params)
        self.assertEqual(result, 222)

    def test_get_cpu_usage_data_1(self):
        configs = {"db": 'run', "table": '123'}
        params = {"project": '111', "device_id": '222', "data_type": "cpu_usage"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'), \
             mock.patch(NAMESPACE + '.get_sys_cpu_usage_data', return_value=111):
            key = MsProfExportDataUtils()
            result = key._get_cpu_usage_data(configs, params)
        self.assertEqual(result, 111)

    def test_get_cpu_usage_data_2(self):
        configs = {"db": 'trace', "table": '123'}
        params = {"project": '333', "device_id": '444', "data_type": "process_cpu_usage"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'), \
             mock.patch(NAMESPACE + '.get_process_cpu_usage', return_value=222):
            key = MsProfExportDataUtils()
            result = key._get_cpu_usage_data(configs, params)
        self.assertEqual(result, 222)

    def test_get_cpu_usage_data_3(self):
        configs = {"db": 'step', "table": '123'}
        params = {"project": '333', "device_id": '444', "data_type": "555"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'):
            key = MsProfExportDataUtils()
            result = key._get_cpu_usage_data(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_cpu_top_funcs(self):
        configs = {"db": 'step', "table": '321', "headers": '654'}
        params = {"project": '333', "device_id": '444'}
        with mock.patch(NAMESPACE + '.get_cpu_hot_function', return_value=111):
            key = MsProfExportDataUtils()
            result = key._get_cpu_top_funcs(configs, params)
        self.assertEqual(result, 111)

    def test_get_ts_cpu_top_funcs(self):
        configs = {"db": 'step'}
        params = {"project": '11', "device_id": '22'}
        with mock.patch(NAMESPACE + '.TsCpuReport.get_output_top_function', return_value=11):
            key = MsProfExportDataUtils()
            result = key._get_ts_cpu_top_funcs(configs, params)
        self.assertEqual(result, 11)

    def test_get_cpu_pmu_events_1(self):
        configs = {"db": 'step', "table": '11', "headers": '22'}
        params = {"data_type": "ctrl_cpu_pmu_events", "project": '12'}
        with mock.patch(NAMESPACE + '.get_aictrl_pmu_events', return_value=111):
            key = MsProfExportDataUtils()
            result = key._get_cpu_pmu_events(configs, params)
        self.assertEqual(result, 111)

    def test_get_cpu_pmu_events_2(self):
        configs = {"db": 'time', "table": '111', "headers": '222'}
        params = {"data_type": "ts_cpu_pmu_events", "project": '12'}
        with mock.patch(NAMESPACE + '.get_ts_pmu_events', return_value=222):
            key = MsProfExportDataUtils()
            result = key._get_cpu_pmu_events(configs, params)
        self.assertEqual(result, 222)

    def test_get_cpu_pmu_events_3(self):
        configs = {"db": 'time', "table": '111', "headers": '222'}
        params = {"data_type": "123", "project": '12'}
        key = MsProfExportDataUtils()
        result = key._get_cpu_pmu_events(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_memory_data_1(self):
        configs = {"db": 'time', "table": '111'}
        params = {"device_id": '456', "data_type": "sys_mem", "project": '12'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'), \
             mock.patch(NAMESPACE + '.get_sys_mem_data', return_value=1111):
            key = MsProfExportDataUtils()
            result = key._get_memory_data(configs, params)
        self.assertEqual(result, 1111)

    def test_get_memory_data_2(self):
        configs = {"db": 'run', "table": '111'}
        params = {"device_id": '456', "data_type": "process_mem", "project": '12'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'), \
             mock.patch(NAMESPACE + '.get_process_mem_data', return_value=2222):
            key = MsProfExportDataUtils()
            result = key._get_memory_data(configs, params)
        self.assertEqual(result, 2222)

    def test_get_memory_data_3(self):
        configs = {"db": 'hwts', "table": '111'}
        params = {"device_id": '456', "data_type": "654", "project": '12'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'):
            key = MsProfExportDataUtils()
            result = key._get_memory_data(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_acl_data_1(self):
        configs = {"db": 'res-hwts', "table": '111'}
        params = {"export_type": "timeline", "device_id": '456', "project": '12'}
        with mock.patch(NAMESPACE + '.AclViewer.get_timeline_data', return_value=111):
            key = MsProfExportDataUtils()
            result = key._get_acl_data(configs, params)
        self.assertEqual(result, 111)

    def test_get_acl_data_2(self):
        configs = {"db": 'hwts', "table": '123'}
        params = {"export_type": "summary", "device_id": '456', "project": '12'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='111'), \
             mock.patch(NAMESPACE + '.AclViewer.get_summary_data', return_value=222):
            key = MsProfExportDataUtils()
            result = key._get_acl_data(configs, params)
        self.assertEqual(result, 222)

    def test_get_acl_statistic_data(self):
        configs = {"db": 'hwts', "table": '456'}
        params = {"project": '12', "device_id": '456'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'), \
             mock.patch(NAMESPACE + '.AclViewer.get_acl_statistic_data', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_acl_statistic_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_op_summary_data(self):
        configs = {"db": 'hwts'}
        params = {"project": '123', "device_id": '456', "iter_id": '789'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='321'), \
             mock.patch(NAMESPACE + '.AiCoreOpReport.get_op_summary_data', return_value=321):
            key = MsProfExportDataUtils()
            result = key._get_op_summary_data(configs, params)
        self.assertEqual(result, 321)

    def test_get_l2_cache_data(self):
        configs = {"db": 'hwts', "table": '123', "unused_cols": '456'}
        params = {"project": '321', "device_id": '654', "iter_id": '987'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='321'), \
             mock.patch(NAMESPACE + '.get_l2_cache_data', return_value=([1], 2, 3)), \
             mock.patch(NAMESPACE + '.DataManager.get_op_dict', return_value=True), \
             mock.patch(NAMESPACE + '.add_op_name', return_value=True):
            key = MsProfExportDataUtils()
            result = key._get_l2_cache_data(configs, params)
        self.assertEqual(result, ([1, 'Op Name'], 2, 3))

    def test_get_step_trace_data_1(self):
        configs = {"headers": '111'}
        params = {"job_id": '321', "device_id": '654', "project": '987', "export_type": "timeline"}
        with mock.patch(NAMESPACE + '.StepTraceViewer.get_step_trace_timeline', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_step_trace_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_step_trace_data_2(self):
        configs = {"headers": '123'}
        params = {"job_id": '321', "device_id": '654', "project": '987', "export_type": "summary"}
        with mock.patch(NAMESPACE + '.StepTraceViewer.get_step_trace_summary', return_value=['123', 1, 2]):
            key = MsProfExportDataUtils()
            result = key._get_step_trace_data(configs, params)
        self.assertEqual(result, ('123123', 1, 2))
        with mock.patch(NAMESPACE + '.StepTraceViewer.get_step_trace_summary', side_effect=OSError):
            key = MsProfExportDataUtils()
            result = key._get_step_trace_data(configs, params)
        self.assertEqual(result, '{"data": "", "status": 1, "info": "message error: "}')

    def test_get_op_statistic_data(self):
        configs = {"db": 'hwts', "headers": '123'}
        params = {"project": '987', "device_id": "654"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='456'), \
             mock.patch(NAMESPACE + '.ReportOPCounter.report_op', return_value=321):
            key = MsProfExportDataUtils()
            result = key._get_op_statistic_data(configs, params)
        self.assertEqual(result, 321)

    def test_get_dvpp_data_1(self):
        configs = {"db": 'hwts', "table": '123'}
        params = {"export_type": "timeline", "project": '987', "device_id": "654", "data_type": '123'}
        with mock.patch(NAMESPACE + '.get_dvpp_timeline', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_dvpp_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_dvpp_data_2(self):
        configs = {"db": 'hwts', "table": '321'}
        params = {"export_type": "summary", "project": '987', "device_id": "654", "data_type": "dvpp"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'), \
             mock.patch(NAMESPACE + '.get_peripheral_dvpp_data', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_dvpp_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_dvpp_data_3(self):
        configs = {"db": 'rec-hwts', "table": '321'}
        params = {"export_type": "summary", "project": '987', "device_id": "654", "data_type": "cpdd"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'):
            key = MsProfExportDataUtils()
            result = key._get_dvpp_data(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_nic_data_1(self):
        configs = {"db": 'rec-hwts', "table": '789'}
        params = {"export_type": "timeline", "project": '987', "device_id": "654", "data_type": "cpdd"}
        with mock.patch(NAMESPACE + '.get_network_timeline', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_nic_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_nic_data_2(self):
        configs = {"db": 'rec-hwts', "table": '456'}
        params = {"export_type": "summary", "project": '987', "device_id": "654", "data_type": "nic"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'), \
             mock.patch(NAMESPACE + '.get_peripheral_nic_data', return_value=111):
            key = MsProfExportDataUtils()
            result = key._get_nic_data(configs, params)
        self.assertEqual(result, 111)

    def test_get_nic_data_3(self):
        configs = {"db": 'rec-hwts', "table": '789'}
        params = {"export_type": "summary", "project": '987', "device_id": "654", "data_type": "cpdd"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='123'):
            key = MsProfExportDataUtils()
            result = key._get_nic_data(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_roce_data_1(self):
        configs = {"db": 'hwts', "table": '789'}
        params = {"export_type": "timeline", "project": '987', "device_id": "654", "data_type": "ddcp"}
        with mock.patch(NAMESPACE + '.get_network_timeline', return_value=321):
            key = MsProfExportDataUtils()
            result = key._get_roce_data(configs, params)
        self.assertEqual(result, 321)

    def test_get_roce_data_2(self):
        configs = {"db": 'hwts', "table": '987'}
        params = {"export_type": "summary", "project": '987', "device_id": "654", "data_type": "roce"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='321'), \
             mock.patch(NAMESPACE + '.get_peripheral_nic_data', return_value=222):
            key = MsProfExportDataUtils()
            result = key._get_roce_data(configs, params)
        self.assertEqual(result, 222)

    def test_get_roce_data_3(self):
        configs = {"db": 'hwts', "table": '123'}
        params = {"export_type": "summary", "project": '987', "device_id": "654", "data_type": "cpddd"}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='321'):
            key = MsProfExportDataUtils()
            result = key._get_roce_data(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_hccs_data_1(self):
        config = {"blank": 'space'}
        params = {"export_type": "timeline", "project": '987', "device_id": "654"}
        with mock.patch(NAMESPACE + '.get_hccs_timeline', return_value=902):
            key = MsProfExportDataUtils()
            result = key._get_hccs_data(config, params)
        self.assertEqual(result, 902)

    def test_get_hccs_data_2(self):
        config = {"space": 'blank'}
        params = {"export_type": "summary", "project": '987', "device_id": "654"}
        with mock.patch(NAMESPACE + '.InterConnectionView.get_hccs_data', return_value=132):
            key = MsProfExportDataUtils()
            result = key._get_hccs_data(config, params)
        self.assertEqual(result, 132)

    def test_get_mini_llc_data_1(self):
        sample_config = {"llc_profiling": "bandwidth"}
        params = {"data_type": "cpddd", "project": '987', "device_id": "654"}
        with mock.patch(NAMESPACE + '.get_llc_bandwidth', return_value=456):
            key = MsProfExportDataUtils()
            result = key._MsProfExportDataUtils__get_mini_llc_data(sample_config, params)
        self.assertEqual(result, 456)

    def test_get_mini_llc_data_2(self):
        sample_config = {"llc_profiling": "capacity"}
        params = {"data_type": "llc_aicpu", "project": '987', "device_id": "654"}
        with mock.patch(NAMESPACE + '.llc_capacity_data', return_value=789):
            key = MsProfExportDataUtils()
            result = key._MsProfExportDataUtils__get_mini_llc_data(sample_config, params)
        self.assertEqual(result, 789)

    def test_get_mini_llc_data_3(self):
        sample_config = {"llc_profiling": "blank"}
        params = {"data_type": "space", "project": '987', "device_id": "654"}
        key = MsProfExportDataUtils()
        result = key._MsProfExportDataUtils__get_mini_llc_data(sample_config, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_non_mini_llc_data_1(self):
        sample_config = 123
        params = {"project": '987', "device_id": "654", "data_type": "space"}
        check = {'status': 0, 'info': '', 'data': {'mode': 'read', 'rate': 'MB/s', 'table': [
            {'Mode': 'read', 'Task': 'Average', 'Hit Rate(%)': '0.3403875', 'Throughput(MB/s)': '5142.23645125'},
            {'Mode': 'read', 'Task': 0, 'Hit Rate(%)': 0.696089, 'Throughput(MB/s)': 11818.426411},
            {'Mode': 'read', 'Task': 1, 'Hit Rate(%)': 0.665461, 'Throughput(MB/s)': 8750.519394},
            {'Mode': 'read', 'Task': 2, 'Hit Rate(%)': 0.0, 'Throughput(MB/s)': 0.0},
            {'Mode': 'read', 'Task': 3, 'Hit Rate(%)': 0.0, 'Throughput(MB/s)': 0.0}]}}
        with mock.patch(NAMESPACE + '.get_llc_train_summary'), \
             mock.patch(NAMESPACE + '.json.loads', return_value=check):
            key = MsProfExportDataUtils()
            result = key._MsProfExportDataUtils__get_non_mini_llc_data(sample_config, params)
        self.assertEqual(result, (['Mode', 'Task', 'Hit Rate(%)', 'Throughput(MB/s)'],
                                  [['read', 'Average', '0.3403875', '5142.23645125'],
                                   ['read', 0, 0.696089, 11818.426411],
                                   ['read', 1, 0.665461, 8750.519394],
                                   ['read', 2, 0.0, 0.0],
                                   ['read', 3, 0.0, 0.0]],
                                  5))

    def test_get_non_mini_llc_data_2(self):
        sample_config = 456
        params = {"project": '789', "device_id": "456", "data_type": "space"}
        check = {'status': 1}
        with mock.patch(NAMESPACE + '.get_llc_train_summary'), \
             mock.patch(NAMESPACE + '.json.loads', return_value=check):
            key = MsProfExportDataUtils()
            result = key._MsProfExportDataUtils__get_non_mini_llc_data(sample_config, params)
        self.assertEqual(result, ([],
                                  '{"status": 1, "info": "Failed to get LLC data(space) in non-mini scene."}',
                                  0))

    def test_get_llc_data_1(self):
        sample_config = 123
        params = {"export_type": "timeline", "project": "456", "data_type": "space"}
        with mock.patch(NAMESPACE + '.get_llc_timeline', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_llc_data(sample_config, params)
        self.assertEqual(result, 123)

    def test_get_llc_data_2(self):
        sample_config = 456
        params = {"export_type": "summary", "project": "456", "data_type": "space"}
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value=111), \
             mock.patch(NAMESPACE + '.MsProfExportDataUtils._MsProfExportDataUtils__get_mini_llc_data',
                        return_value=333):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            key = MsProfExportDataUtils()
            result = key._get_llc_data(sample_config, params)
        self.assertEqual(result, 333)

    def test_get_llc_data_3(self):
        sample_config = 789
        params = {"export_type": "export", "project": "456", "data_type": "space"}
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value=True), \
             mock.patch(NAMESPACE + '.MsProfExportDataUtils._MsProfExportDataUtils__get_non_mini_llc_data',
                        return_value=444):
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            key = MsProfExportDataUtils()
            result = key._get_llc_data(sample_config, params)
        self.assertEqual(result, 444)

    def test_get_llc_data_4(self):
        sample_config = 987
        params = {"export_type": "export_data", "project": "456", "data_type": "space"}
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value=False):
            key = MsProfExportDataUtils()
            result = key._get_llc_data(sample_config, params)
        self.assertEqual(result, ([], '{"status": 1, "info": "Failed to get LLC sample config data(space)."}', 0))

    def test_get_hbm_data_1(self):
        configs = {"headers": 123}
        params = {"export_type": "timeline", "project": "456", "device_id": '123', "data_type": "space"}
        with mock.patch(NAMESPACE + '.get_hbm_timeline', return_value='ada'):
            key = MsProfExportDataUtils()
            result = key._get_hbm_data(configs, params)
        self.assertEqual(result, 'ada')

    def test_get_hbm_data_2(self):
        configs = {"headers": 321}
        params = {"export_type": "summary", "project": "456", "device_id": '123', "data_type": "space"}
        with mock.patch(NAMESPACE + '.get_hbm_summary_data', return_value=[1, 2, 3]):
            key = MsProfExportDataUtils()
            result = key._get_hbm_data(configs, params)
        self.assertEqual(result, (321, [1, 2, 3], 3))

    def test_get_hbm_data_3(self):
        configs = {"headers": 456}
        params = {"export_type": "data", "project": "456", "device_id": '123', "data_type": "space"}
        with mock.patch(NAMESPACE + '.get_hbm_summary_data', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_hbm_data(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_hbm_data_4(self):
        configs = {"headers": 654}
        params = {"export_type": "exported", "project": "456", "device_id": '123', "data_type": "space"}
        with mock.patch(NAMESPACE + '.get_hbm_summary_data', return_value=False):
            key = MsProfExportDataUtils()
            result = key._get_hbm_data(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_pcie_data_1(self):
        configs = 'ada'
        params = {"export_type": "timeline", "project": "456", "device_id": '123'}
        with mock.patch(NAMESPACE + '.get_pcie_timeline', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_pcie_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_pcie_data_2(self):
        configs = '121'
        params = {"export_type": "summary", "project": "456", "device_id": '123'}
        with mock.patch(NAMESPACE + '.InterConnectionView.get_pcie_summary_data', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_pcie_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_aicpu_data(self):
        configs = {'headers': '212'}
        params = {"iter_id": "summary", "project": "456", "device_id": '123'}
        with mock.patch(NAMESPACE + '.ParseAiCpuData.analysis_aicpu', return_value=(1, [1, 2])):
            key = MsProfExportDataUtils()
            result = key._get_aicpu_data(configs, params)
        self.assertEqual(result, (1, [1, 2], 2))

    def test_get_dp_data_1(self):
        configs = {'headers': '10'}
        params = {"iter_id": "timeline", "project": "456", "device_id": '123'}
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir', return_value=123), \
             mock.patch(NAMESPACE + '.ParseDpData.analyse_dp', return_value={}):
            key = MsProfExportDataUtils()
            result = key._get_dp_data(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_dp_data_2(self):
        configs = {'headers': '10'}
        params = {"iter_id": "timeline", "project": "456", "device_id": '123'}
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir', return_value=123), \
             mock.patch(NAMESPACE + '.ParseDpData.analyse_dp', return_value={'test': 123}):
            key = MsProfExportDataUtils()
            key._get_dp_data(configs, params)

    def test_get_fusion_op_data(self):
        configs = {"db": 123, "table": 321}
        params = {"project": "456", "device_id": '123'}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=123), \
             mock.patch(NAMESPACE + '.get_ge_model_data', return_value=555):
            key = MsProfExportDataUtils()
            result = key._get_fusion_op_data(configs, params)
        self.assertEqual(result, 555)

    def test_get_ai_core_sample_based_data_1(self):
        configs = {"db": 123}
        params = {"export_type": "timeline", "project": "456", "device_id": '123'}
        with mock.patch(NAMESPACE + '.get_aicore_utilization_timeline', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_ai_core_sample_based_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_ai_core_sample_based_data_2(self):
        configs = {"db": 321}
        params = {"export_type": "summary", "project": "456", "device_id": '123'}
        with mock.patch(NAMESPACE + '.get_core_sample_data', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_ai_core_sample_based_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_aiv_sample_based_data(self):
        configs = {"db": 456}
        params = {"project": "456", "device_id": '123'}
        with mock.patch(NAMESPACE + '.get_core_sample_data', return_value=123):
            key = MsProfExportDataUtils()
            result = key._get_aiv_sample_based_data(configs, params)
        self.assertEqual(result, 123)

    def test_get_host_cpu_usage_data(self):
        configs = {"bd": 456}
        params = {"project": "456",
                  StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE}
        with mock.patch(NAMESPACE + '.get_host_prof_timeline', return_value=111):
            InfoConfReader()._info_json = {"pid": 123}
            key = MsProfExportDataUtils()
            result = key._get_host_cpu_usage_data(configs, params)
        self.assertEqual(result, 111)

    def test_get_host_mem_usage_data(self):
        configs = {"rome": 456}
        params = {"project": "654",
                  StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE}
        with mock.patch(NAMESPACE + '.get_host_prof_timeline', return_value=111):
            InfoConfReader()._info_json = {"pid": 123}
            key = MsProfExportDataUtils()
            result = key._get_host_mem_usage_data(configs, params)
        self.assertEqual(result, 111)

    def test_get_host_network_usage_data(self):
        configs = {"eng": 456}
        params = {"project": "789",
                  StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE}
        with mock.patch(NAMESPACE + '.get_host_prof_timeline', return_value=111):
            InfoConfReader()._info_json = {"pid": 123}
            key = MsProfExportDataUtils()
            result = key._get_host_network_usage_data(configs, params)
        self.assertEqual(result, 111)

    def test_get_host_disk_usage_data(self):
        configs = {"eng": 654}
        params = {"project": "987",
                  StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE}
        with mock.patch(NAMESPACE + '.get_host_prof_timeline', return_value=111):
            InfoConfReader()._info_json = {"pid": 123}
            key = MsProfExportDataUtils()
            result = key._get_host_disk_usage_data(configs, params)
        self.assertEqual(result, 111)

    def test_get_host_runtime_api_1(self):
        configs = {"headers": 654}
        params = {"export_type": "summary", "project": "987"}
        with mock.patch(NAMESPACE + '.HostSyscallPresenter.get_summary_api_info', return_value=[1]):
            key = MsProfExportDataUtils()
            result = key._get_host_runtime_api(configs, params)
        self.assertEqual(result, (654, [1], 1))

    def test_get_host_runtime_api_2(self):
        configs = {"headers": 654}
        params = {"export_type": "summary", "project": "987"}
        with mock.patch(NAMESPACE + '.HostSyscallPresenter.get_summary_api_info', return_value=False):
            key = MsProfExportDataUtils()
            result = key._get_host_runtime_api(configs, params)
        self.assertEqual(result, ([], [], 0))

    def test_get_host_runtime_api_3(self):
        configs = {"headers": 789}
        params = {"export_type": "timeline", "project": "987"}
        with mock.patch(NAMESPACE + '.HostSyscallPresenter.get_summary_api_info', return_value=False), \
             mock.patch(NAMESPACE + '.get_host_prof_timeline', return_value=1):
            key = MsProfExportDataUtils()
            result = key._get_host_runtime_api(configs, params)
        self.assertEqual(result, 1)

    def test_get_ge_data_1(self):
        sample_configs = {"test": 1}
        params = {"export_type": "timeline", "project": 1}
        with mock.patch(NAMESPACE + '.get_ge_timeline_data', return_value=1):
            key = MsProfExportDataUtils()
            result = key._get_ge_data(sample_configs, params)
        self.assertEqual(result, 1)

    def test_get_ge_data_2(self):
        sample_configs = {"test": 2}
        params = {"export_type": "summary", "project": 1}
        key = MsProfExportDataUtils()
        result = key._get_ge_data(sample_configs, params)
        self.assertEqual(result, '{"status": 2, "info": "Please check params, '
                                 'Currently ge data does not support exporting files other than timeline."}')

    def test_get_bulk_data_1(self):
        sample_configs = {"test": 2}
        params = {"export_type": "timeline"}
        with mock.patch('msinterface.msprof_timeline.StepTraceViewer.get_one_iter_timeline_data',
                        return_value=json.dumps({'a': 1})):
            InfoConfReader()._info_json = {"pid": 123}
            key = MsProfExportDataUtils()
            result = key._get_bulk_data(sample_configs, params)
        self.assertEqual(result, '')

    def test_get_bulk_data_2(self):
        sample_configs = {"test": 2}
        params = {"export_type": "summary"}
        InfoConfReader()._info_json = {"pid": 123}
        key = MsProfExportDataUtils()
        result = key._get_bulk_data(sample_configs, params)
        self.assertEqual(result, '{"status": 2, "info": "Please check params, '
                                 'Currently bulk data export params should be timeline."}')

    def test_get_bulk_data_should_return_timeline_when_input_valid_data(self):
        sample_configs = {"test": 2}
        params = {"export_type": "timeline"}
        data = json.dumps([
            {'name': 'process_name', 'pid': 0, 'tid': 0, 'args': {'name': 'Step Trace'}, 'ph': 'M'},
            {'name': 'Reduce', 'pid': 0, 'ph': 'X', 'ts': 1210122}])
        with mock.patch('viewer.pipeline_overlap_viewer.PipelineOverlapViewer.get_timeline_data',
                        return_value=data), \
             mock.patch('msinterface.msprof_timeline.StepTraceViewer.get_one_iter_timeline_data',
                        return_value=json.dumps([])):
            InfoConfReader()._info_json = {"pid": 123}
            key = MsProfExportDataUtils()
            result = key._get_bulk_data(sample_configs, params)
        expected = '[{"name": "process_name", "pid": 3, "tid": 0, "args": {"name": "Ascend ' \
                   'Hardware"}, "ph": "M"}, {"name": "Reduce", "pid": 3, "ph": "X", "ts": ' \
                   '1210122}, {"name": "process_labels", "pid": 3, "tid": 0, "args": {"labels": ' \
                   '"NPU"}, "ph": "M"}, {"name": "process_sort_index", "pid": 3, "tid": 0, ' \
                   '"args": {"sort_index": 3}, "ph": "M"}]'
        self.assertEqual(expected, result)
        InfoConfReader()._info_json = {}

    def test_get_bulk_data_should_return_empty_when_input_invalid_data(self):
        sample_configs = {"test": 1}
        params = {"export_type": "timeline"}
        data = "invalid_data"
        with mock.patch('viewer.pipeline_overlap_viewer.PipelineOverlapViewer.get_timeline_data',
                        return_value=data):
            InfoConfReader()._info_json = {"pid": 123}
            key = MsProfExportDataUtils()
            InfoConfReader()._info_json = {}
            self.assertEqual("", key._get_bulk_data(sample_configs, params))

    def test_get_task_time(self):
        sample_configs = {"test": 2}
        params = {"export_type": "summary"}
        with mock.patch(NAMESPACE + '.FftsLogViewer.get_timeline_data', return_value=1):
            key = MsProfExportDataUtils()
            result = key._get_sub_task_time(sample_configs, params)
        self.assertEqual(result, 1)

    def test_get_hccl_timeline(self):
        sample_configs = {"test": 2}
        summary_params = {"export_type": "summary"}
        timeline_params = {"export_type": "timeline"}
        with mock.patch(NAMESPACE + '.HCCLExport.get_hccl_timeline_data', return_value=1):
            key = MsProfExportDataUtils()
            InfoConfReader()._info_json = {'pid': 1}
            result = key._get_hccl_timeline(sample_configs, timeline_params)
        self.assertEqual(result, 1)
        result = key._get_hccl_timeline(sample_configs, summary_params)
        self.assertEqual(result,
                         '{"status": 2, "info": "Please check params, Currently hccl data does not support exporting files other than timeline."}')

    def test_get_msproftx_data(self):
        sample_configs = {"test": 2}
        params = {"export_type": "summary"}
        with mock.patch(NAMESPACE + '.MsprofTxViewer.get_summary_data', return_value=('test', [1], 1)):
            key = MsProfExportDataUtils()
            result = key._get_msproftx_data(sample_configs, params)
        self.assertEqual(result, ('test', [1], 1))
        with mock.patch(NAMESPACE + '.MsprofTxViewer.get_timeline_data', return_value=''):
            key = MsProfExportDataUtils()
            result = key._get_msproftx_data(sample_configs, {"export_type": "timeline"})
        self.assertEqual(result, '')

    def test_get_ge_op_execute_data(self):
        sample_configs = {"test": 2}
        with mock.patch(NAMESPACE + '.GeOpExecuteViewer.get_summary_data', return_value=('test', [1], 1)):
            key = MsProfExportDataUtils()
            result = key._get_ge_op_execute_data(sample_configs, {"export_type": "summary"})
        self.assertEqual(result, ('test', [1], 1))
        with mock.patch(NAMESPACE + '.GeOpExecuteViewer.get_timeline_data', return_value=''):
            key = MsProfExportDataUtils()
            result = key._get_ge_op_execute_data(sample_configs, {"export_type": "timeline"})
        self.assertEqual(result, '')

    def test_get_stars_soc_data(self):
        InfoConfReader()._info_json = {"pid": 123}
        with mock.patch(NAMESPACE + '.StarsSocView.get_timeline_data', return_value={}):
            res = MsProfExportDataUtils._get_stars_soc_data({}, {})
        self.assertEqual(res, {})

    def test_get_stars_chip_trans_data(self):
        InfoConfReader()._info_json = {"pid": 123}
        with mock.patch(NAMESPACE + '.StarsChipTransView.get_timeline_data', return_value={}):
            res = MsProfExportDataUtils._get_stars_chip_trans_data({}, {})
        self.assertEqual(res, {})

    def test_get_pytorch_operator_data(self):
        sample_configs = {"test": 2}
        params = {"export_type": "summary"}
        with mock.patch(NAMESPACE + '.MsprofTxViewer.get_pytorch_operator_data', return_value=('test', [1], 1)):
            key = MsProfExportDataUtils()
            result = key._get_pytorch_operator_data(sample_configs, params)
        self.assertEqual(result, ('test', [1], 1))


if __name__ == '__main__':
    unittest.main()
