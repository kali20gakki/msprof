import unittest
from unittest import mock
from viewer.interface.base_viewer import BaseViewer
from msmodel.stars.ffts_log_model import FftsLogModel

NAMESPACE = 'viewer.interface.base_viewer'


class TestBaseViewer(unittest.TestCase):

    def test_get_timeline_data(self):
        configs = {}
        params = {'data_type': 'ffts_thread_log', 'export_type': 'timeline'}
        with mock.patch('msmodel.stars.ffts_log_model.FftsLogModel.check_db', return_value=1),\
                mock.patch('msmodel.stars.ffts_log_model.FftsLogModel.get_timeline_data',
                           return_value=[1]), \
                mock.patch('msmodel.stars.ffts_log_model.FftsLogModel.finalize'), \
                mock.patch(NAMESPACE + '.BaseViewer.get_trace_timeline', return_value=1), \
                mock.patch('viewer.get_trace_timeline.TraceViewer.format_trace_events', return_value=1):
            check = BaseViewer(configs, params)
            check.model_list = {'ffts_thread_log': FftsLogModel}
            ret = check.get_timeline_data()
            self.assertEqual(ret, 1)
        with mock.patch(NAMESPACE + '.BaseViewer.get_model_instance', return_value=None):
            check = BaseViewer(configs, params)
            check.model_list = {'ffts_thread_log': FftsLogModel}
            ret = check.get_timeline_data()
            self.assertEqual(ret, 'null')

    def test_get_summary_data(self):
        configs = {'headers': [1]}
        params = {'data_type': 'ffts_thread_log'}
        with mock.patch('msmodel.stars.ffts_log_model.FftsLogModel.check_db', return_value=1), \
                mock.patch('msmodel.stars.ffts_log_model.FftsLogModel.get_summary_data',
                           return_value=[1]), \
                mock.patch('msmodel.stars.ffts_log_model.FftsLogModel.finalize'):
            check = BaseViewer(configs, params)
            check.model_list = {'ffts_thread_log': FftsLogModel}
            ret = check.get_summary_data()
            self.assertEqual(ret, ([1], [1], 1))
        with mock.patch(NAMESPACE + '.BaseViewer.get_model_instance', return_value=None):
            check = BaseViewer(configs, params)
            check.model_list = {'ffts_thread_log': FftsLogModel}
            ret = check.get_summary_data()
            self.assertEqual(ret, ([1], [], 0))

    def test_get_model_instance(self):
        configs = {}
        params = {'data_type': 'ffts_thread_log'}
        check = BaseViewer(configs, params)
        check.model_list = {'ffts_thread_log': FftsLogModel}
        check.get_model_instance()

    def test_get_trace_timeline(self):
        pass


if __name__ == '__main__':
    unittest.main()
