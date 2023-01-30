import unittest
from unittest import mock

from viewer.msproftx_viewer import MsprofTxViewer

NAMESPACE = 'viewer.peripheral_report'


class TestMsprofTxViewer(unittest.TestCase):
    configs = {'handler': '_get_msproftx_data',
               'headers': ['pid', ' tid', ' category', ' event_type', ' payload_type', ' payload_value', ' Start_time',
                           ' End_time', ' message_type', ' message'], 'db': 'msproftx.db', 'table': 'MsprofTx',
               'unused_cols': []}
    params = {'data_type': 'msprof_tx', 'project': 'D:\\Job\\analyzer_testcase\\origin_data\\1910_mini_no_ai_core',
              'device_id': '0', 'job_id': 'job_default', 'export_type': 'summary', 'iter_id': 1, 'export_format': 'csv',
              'model_id': 1}

    def test_get_summary_data(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.get_summary_data',
                        return_value=((0, 0, 0, 0, 0, 0, 0, 5, 0, 'test'), (0, 1, 0, 0, 1, 0, 1, 7, 0, 'test'))):
            result = MsprofTxViewer(self.configs, self.params).get_summary_data()
            self.assertEqual(result, (['pid', ' tid', ' category', ' event_type', ' payload_type',
                                       ' payload_value', ' Start_time', ' End_time', ' message_type', ' message'],
                                      ((0, 0, 0, 0, 0, 0, 0, 5, 0, 'test'), (0, 1, 0, 0, 1, 0, 1, 7, 0, 'test')),
                                      2))

    def test_get_timeline_data(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.get_timeline_data',
                        return_value=((0, 0, 0, 0, 0, 0, 0, 5, 0, 'test'), (0, 1, 0, 0, 1, 0, 1, 7, 0, 'test'))):
            result = MsprofTxViewer(self.configs, self.params).get_timeline_data()
            self.assertEqual(type(result), str)


if __name__ == '__main__':
    unittest.main()
