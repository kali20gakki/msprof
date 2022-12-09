import unittest
from collections import OrderedDict

from viewer.stars.acc_pmu_viewer import AccPmuViewer


class AccPmuTestData:

    def __init__(self, *args):
        self.timestamp = args[0]
        self.read_bandwidth = args[1]
        self.acc_id = args[2]
        self.write_bandwidth = args[3]
        self.read_ost = args[4]
        self.write_ost = args[5]


class TestAccPmuViewer(unittest.TestCase):
    def test_get_timeline_header(self):
        param = {"data_type": "acc_pmu"}
        check = AccPmuViewer({}, param)
        ret = check.get_timeline_header()
        self.assertEqual([["process_name", 0, 0, "acc_pmu"]], ret)

    def test_get_trace_timeline(self):
        check = AccPmuViewer({}, {})
        ret = check.get_trace_timeline([])
        self.assertEqual([], ret)

        param = {"data_type": "acc_pmu"}
        datas = [AccPmuTestData(1, 2, 3, 4, 5, 6, 7, 8, 9, 123)]
        check = AccPmuViewer({}, param)
        ret = check.get_trace_timeline(datas)
        self.assertEqual(5, len(ret))
