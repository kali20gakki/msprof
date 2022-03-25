import unittest
from collections import OrderedDict

from viewer.stars.acc_pmu_viewer import AccPmuViewer


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
        datas = [[1, 2, 3, 4, 5, 6, 7, 8, 9, 123]]
        check = AccPmuViewer({}, param)
        ret = check.get_trace_timeline(datas)
        expect_result = [OrderedDict([('name', 'process_name'),
                                      ('pid', 0),
                                      ('tid', 0),
                                      ('args', OrderedDict([('name', 'acc_pmu')])),
                                      ('ph', 'M')]),
                         OrderedDict([('name', 'acc_id 3'),
                                      ('ts', 123),
                                      ('pid', 0),
                                      ('tid', 0),
                                      ('args',
                                       {'read_bandwidth': 6,
                                        'read_ost': 8,
                                        'write_band_width': 7,
                                        'write_ost': 9}),
                                      ('ph', 'C')])]
        self.assertEqual(expect_result, ret)
