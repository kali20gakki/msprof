import unittest

from common_func.info_conf_reader import InfoConfReader
from viewer.stars.sio_viewer import SioViewer


class TestSioViewer(unittest.TestCase):
    def test_get_trace_timeline(self):
        datas = [
            (0, 1, 2, 3, 4, 5, 6, 7, 8, 214),
            (1, 1, 2, 3, 4, 5, 6, 7, 8, 214),
            (0, 1, 2, 3, 4, 5, 6, 7, 8, 216),
            (1, 1, 2, 3, 4, 5, 6, 7, 8, 216)
        ]
        InfoConfReader()._info_json = {"pid": '0'}
        check = SioViewer({}, {})
        ret = check.get_trace_timeline(datas)
        self.assertEqual(17, len(ret))
