import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from viewer.qos_viewer import QosViewer

NAMESPACE = 'msmodel.hardware.qos_viewer_model'


class TestQosViewer(unittest.TestCase):
    def test_get_trace_timeline(self):
        InfoConfReader()._info_json = {"devices": '0', "pid": 1, "tid": 1}
        configs = {"table": "QosOriginalData"}
        param = {"data_type": "qos"}
        datas = [
            [1713923176576148000.0, 284, 0, "read"],
            [1713923176576348000.0, 10, 0, "write"],
            [1713923176576548000.0, 23, 0, "read"],
            [1713923176576748000.0, 35, 0, "write"]
        ]
        with mock.patch(NAMESPACE + '.QosViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.QosViewModel.get_qos_info', return_value=[[0, "qos_stream_stub"]]):
            check = QosViewer(configs, param)
            ret = check.get_trace_timeline(datas)
            self.assertEqual(5, len(ret))
