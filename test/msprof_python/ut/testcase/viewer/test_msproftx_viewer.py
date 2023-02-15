import unittest
from unittest import mock
from collections import OrderedDict

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
        top_down_data = mock.Mock()
        top_down_data.category = 1
        top_down_data.payload_type = 2
        top_down_data.payload_value = 3
        top_down_data.message_type = 4
        top_down_data.event_type = 5
        top_down_data.call_stack = 6
        top_down_data.message = 7
        top_down_data.pid = 8
        top_down_data.tid = 9
        top_down_data.start_time = 0
        top_down_data.dur_time = 0

        msproftx_data = (top_down_data,)

        expect_res_dict = OrderedDict([
            ("Category", str(1)),
            ("Payload_type", 2),
            ("Payload_value", 3),
            ("Message_type", 4),
            ("event_type", 5),
            ("call_stack", 6)
        ])

        trace_data_msproftx = [[7, 8, 9, 0, 0, expect_res_dict]]

        result = MsprofTxViewer.format_data(msproftx_data)
        self.assertEqual(result, trace_data_msproftx)

if __name__ == '__main__':
    unittest.main()
