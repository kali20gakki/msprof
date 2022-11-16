import unittest

from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from profiling_bean.stars.stars_common import StarsCommon

NAMESPACE = 'profiling_bean.struct_info.stars_common'


class TestStarsCommon(unittest.TestCase):

    def test_init(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        self.assertEqual(check._stream_id, 2)

    def test_task_id(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        ret = check.task_id
        self.assertEqual(ret, 1)

    def test_stream_id(self):
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        self.assertEqual(check.stream_id, 2)

    def test_timestamp(self):
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[
            DeviceInfo(hwts_frequency='50', aic_frequency='1500', aiv_frequency='1500').device_info])).process()
        task_id, stream_id, timestamp = 1, 2, 3
        check = StarsCommon(task_id, stream_id, timestamp)
        self.assertEqual(check.timestamp, 59.99999999999999)


if __name__ == '__main__':
    unittest.main()
