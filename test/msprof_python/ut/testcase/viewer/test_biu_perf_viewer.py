import unittest
from unittest import mock
from collections import OrderedDict
from viewer.biu_perf_viewer import BiuPerfViewer

NAMESPACE = 'viewer.biu_perf_viewer.'


class TestBiuPerfViewer(unittest.TestCase):

    def setUp(self) -> None:
        profect_path = ""
        self.biu_perf_viewer = BiuPerfViewer(profect_path)

    def test_get_timeline(self) -> None:
        expect_res = '"meta_timeline biu_flow_timeline biu_cycles_timeline "'

        meta_timeline = "meta_timeline "
        biu_flow_timeline = "biu_flow_timeline "
        biu_cycles_timeline = "biu_cycles_timeline "

        with mock.patch(NAMESPACE + "BiuPerfViewer.get_meta_timeline", return_value=meta_timeline), \
            mock.patch(NAMESPACE + "BiuPerfViewer.get_biu_flow_timeline", return_value=biu_flow_timeline), \
            mock.patch(NAMESPACE + "BiuPerfViewer.get_biu_cycles_timeline", return_value=biu_cycles_timeline):
            res = self.biu_perf_viewer.get_timeline()

        self.assertEqual(expect_res, res)

    def test_get_meta_timeline(self) -> None:
        biu_flow_process = "biu_flow_process"
        biu_flow_thread = "biu_flow_thread"
        biu_cycles_process = "biu_cycles_process"
        biu_cycles_thread = "biu_cycles_thread"

        metadata_event = "BIU"

        expect_res = [metadata_event] * 4

        with mock.patch(NAMESPACE + "BiuPerfModel.get_biu_flow_process", return_value=biu_flow_process), \
            mock.patch(NAMESPACE + "BiuPerfModel.get_biu_flow_thread", return_value=biu_flow_thread), \
            mock.patch(NAMESPACE + "BiuPerfModel.get_biu_cycles_process", return_value=biu_cycles_process), \
            mock.patch(NAMESPACE + "BiuPerfModel.get_biu_cycles_thread", return_value=biu_cycles_thread), \
            mock.patch(NAMESPACE + "TraceViewManager.metadata_event", return_value=[metadata_event]):
            res = self.biu_perf_viewer.get_meta_timeline()

        self.assertEqual(expect_res, res)

    def test_get_biu_flow_timeline(self) -> None:
        expect_res = [OrderedDict([('name', 'bandawith'),
                      ('ts', 20),
                      ('pid', 3),
                      ('tid', 4),
                      ('args', OrderedDict([('flow', 50)])),
                      ('ph', 'C')])]

        biu_flow_data = [["bandawith", 20, 3, 4, 50]]
        with mock.patch(NAMESPACE + "BiuPerfModel.get_biu_flow_data", return_value=biu_flow_data):
            res = self.biu_perf_viewer.get_biu_flow_timeline()
        self.assertEqual(expect_res, res)

    def test_get_biu_cycles_timeline(self) -> None:
        expect_res = [OrderedDict([('name', ''),
                      ('pid', 1),
                      ('tid', 2),
                      ('ts', 300),
                      ('dur', 100),
                      ('args', OrderedDict([('cycle_num', 50), ("ratio", 0.5)])),
                      ('ph', 'X')])]

        biu_flow_data = [[1, 2, 300, 100, 50, 0.5]]
        with mock.patch(NAMESPACE + "BiuPerfModel.get_biu_cycles_data", return_value=biu_flow_data):
            res = self.biu_perf_viewer.get_biu_cycles_timeline()
        self.assertEqual(expect_res, res)
