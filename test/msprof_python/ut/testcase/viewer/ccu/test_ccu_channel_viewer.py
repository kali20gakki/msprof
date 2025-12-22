import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.ccu.ccu_channel_dto import OriginChannelDto
from viewer.ccu.ccu_channel_viewer import CCUChannelViewer

NAMESPACE = 'msmodel.hardware.ccu_channel_model'


class TestCCUChannelViewer(unittest.TestCase):

    def test_format_channel_summary_data_should_return_formatted_data(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        datas = [
            OriginChannelDto(1, 4919982785935, 25011, 25011, 25011),
            OriginChannelDto(2, 4919982785938, 13185, 13185, 13185),
            OriginChannelDto(3, 4919982785938, 0, 0, 0)
        ]
        with mock.patch(NAMESPACE + '.CCUViewerChannelModel.check_table', return_value=True):
            check = CCUChannelViewer({}, {})
            ret = check.format_channel_summary_data(datas)
            self.assertEqual(3, len(ret))
            self.assertEqual(1, ret[0][0])
            self.assertEqual(0, ret[2][2])
