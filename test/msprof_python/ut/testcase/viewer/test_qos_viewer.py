import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from viewer.qos_viewer import QosViewer

NAMESPACE = 'msmodel.hardware.qos_viewer_model'


class TestQosViewer(unittest.TestCase):
    def test_get_trace_timeline(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1}
        InfoConfReader()._sample_json = {"qosEvents": "SDMA_QOS,DVPP_VENC_QOS"}
        configs = {"table": "QosOriginalData"}
        param = {"data_type": "qos"}
        datas = [
            [1713923176576148000.0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0],
            [1713923176576348000.0, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0]
        ]
        with mock.patch(NAMESPACE + '.QosViewModel.check_table', return_value=True):
            check = QosViewer(configs, param)
            ret = check.get_trace_timeline(datas)
            self.assertEqual(5, len(ret))
