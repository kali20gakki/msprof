import unittest
from collections import OrderedDict

from viewer.stars.inter_soc_viewer import InterSocViewer


class TestInterSocViewer(unittest.TestCase):
    def test_get_timeline_header(self):
        param = {"data_type": "inter_soc"}
        check = InterSocViewer({}, param)
        ret = check.get_timeline_header()
        self.assertEqual([["process_name", 0, 0, "inter_soc"]], ret)

    def test_get_trace_timeline(self):
        check = InterSocViewer({}, {})
        ret = check.get_trace_timeline([])
        self.assertEqual([], ret)

        param = {"data_type": "inter_soc"}
        datas = [[1, 1, 123], [2, 1, 124]]
        check = InterSocViewer({}, param)
        ret = check.get_trace_timeline(datas)
        expect_result = [OrderedDict([('name', 'process_name'),
                                      ('pid', 0),
                                      ('tid', 0),
                                      ('args', OrderedDict([('name', 'inter_soc')])),
                                      ('ph', 'M')]),
                         OrderedDict([('name', 'Buffer BW Level'),
                                      ('ts', 123),
                                      ('pid', 0),
                                      ('tid', 0),
                                      ('args', {'Value': 1}),
                                      ('ph', 'C')]),
                         OrderedDict([('name', 'Mata BW Level'),
                                      ('ts', 123),
                                      ('pid', 0),
                                      ('tid', 0),
                                      ('args', {'Value': 1}),
                                      ('ph', 'C')]),
                         OrderedDict([('name', 'Buffer BW Level'),
                                      ('ts', 124),
                                      ('pid', 0),
                                      ('tid', 0),
                                      ('args', {'Value': 2}),
                                      ('ph', 'C')]),
                         OrderedDict([('name', 'Mata BW Level'),
                                      ('ts', 124),
                                      ('pid', 0),
                                      ('tid', 0),
                                      ('args', {'Value': 1}),
                                      ('ph', 'C')])]
        self.assertEqual(expect_result, ret)
