import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from viewer.stars.ffts_log_viewer import FftsLogViewer

NAMESPACE = 'viewer.stars.ffts_log_viewer'


class TestFftsLogViewer(unittest.TestCase):

    def test_get_time_timeline_header(self):
        data = (['thread', 2, "3", 4, 5],)
        configs, params = {}, {}
        InfoConfReader()._info_json = {'pid': 1, 'tid': 0}
        check = FftsLogViewer(configs, params)
        ret = check.get_time_timeline_header(data)
        self.assertEqual(ret, [['process_name', 1, 0, 'subtask_time'],
                               ['thread_name', 2, '3', '3']])

    def test_get_trace_timeline(self):
        data = {}
        configs, params = {}, {}
        check = FftsLogViewer(configs, params)
        ret = check.get_trace_timeline(data)
        self.assertEqual(ret, [])
        data = {"thread_data_list": [], "subtask_data_list": []}
        with mock.patch('common_func.trace_view_manager.TraceViewManager.time_graph_trace', return_value=[]), \
             mock.patch('common_func.trace_view_manager.TraceViewManager.metadata_event', return_value=[]), \
             mock.patch(NAMESPACE + '.FftsLogViewer.get_time_timeline_header', return_value=()):
            check = FftsLogViewer(configs, params)
            ret = check.get_trace_timeline(data)
            self.assertEqual(ret, [])


if __name__ == '__main__':
    unittest.main()
