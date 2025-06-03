import unittest
from unittest import mock

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.ms_constant.number_constant import NumberConstant


NAMESPACE = 'common_func.ai_stack_data_check_manager'


class TestAiStackDataCheckManager(unittest.TestCase):

    def test_is_device_exist_should_return_true_when_contains_dir_ends_with_device(self):
        with mock.patch('os.listdir', return_value=['device_1']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertTrue(result)

    def test_is_device_exist_should_return_false_when_contains_dir_ends_with_host(self):
        with mock.patch('os.listdir', return_value=['host']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertFalse(result)

    def test_is_device_exist_should_return_false_when_contains_dir_ends_with_timeline(self):
        with mock.patch('os.listdir', return_value=['timeline']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertFalse(result)

    def test_is_device_exist_should_return_false_when_contains_dir_ends_with_timeline_and_host(self):
        with mock.patch('os.listdir', return_value=['host', 'timeline']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertFalse(result)

    def test_is_device_exist_should_return_true_when_contains_dir_ends_with_device_and_host(self):
        with mock.patch('os.listdir', return_value=['device_1', 'host']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertTrue(result)

    def test_check_outputs_should_return_true_when_device_id_not_equal_host_id(self):
        result = AiStackDataCheckManager._check_output('/path/to/result_dir', device_id=2)
        self.assertTrue(result)

    def test_check_output_should_return_false_when_result_dir_ends_with_device(self):
        result = AiStackDataCheckManager._check_output('/path/to/result_dir/device_1', device_id=NumberConstant.HOST_ID)
        self.assertFalse(result)

    def test_check_output_should_return_false_when_result_dir_ends_with_host_and_device_subdir_exists(self):
        with mock.patch('os.listdir', return_value=['device_1']):
            result = AiStackDataCheckManager._check_output('/path/to/result_dir/host', device_id=NumberConstant.HOST_ID)
            self.assertFalse(result)

    def test_check_output_should_return_true_when_result_dir_ends_with_host_but_device_subdir_not_exist(self):
        with mock.patch('os.listdir', return_value=['other_subdir']):
            result = AiStackDataCheckManager._check_output('/path/to/result_dir/host', device_id=NumberConstant.HOST_ID)
            self.assertTrue(result)

    def test_export_summary_csv_with_c_should_return_false_when_so_exist(self):
        with mock.patch('common_func.data_check_manager.DataCheckManager.check_export_with_so',
                        return_value=True):
            result_api_statistic = AiStackDataCheckManager.contain_api_statistic_data('/path/to/result_dir/host',
                                                                        device_id=NumberConstant.HOST_ID)
            self.assertFalse(result_api_statistic)
            result_fusion_op = AiStackDataCheckManager.contain_fusion_op_data('/path/to/result_dir/host',
                                                                           device_id=NumberConstant.HOST_ID)
            self.assertFalse(result_fusion_op)