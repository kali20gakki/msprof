import unittest
from unittest import mock
import pytest
import multiprocessing
from msinterface.msprof_monitor import JobDispatcher
from msinterface.msprof_monitor import JobMonitor
from msinterface.msprof_monitor import _dispatch_job
from msinterface.msprof_monitor import _monitor_job
from msinterface.msprof_monitor import monitor

from constant.constant import INFO_JSON
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'msinterface.msprof_monitor'


class TestJobDispatcher(unittest.TestCase):
    # def test_add_symlink(self): XXX
    #     with mock.patch('os.listdir', return_value=['Ross', 'Rachel']), \
    #          mock.patch(NAMESPACE + '.PathManager.get_dispatch_dir', return_value=True):
    #         with mock.patch('os.path.realpath', return_value=True), \
    #              mock.patch('os.path.join', return_value=True), \
    #              mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True):
    #             with mock.patch('os.path.isdir', return_value=True):
    #                 key = JobDispatcher('123')
    #                 key.add_symlink()
    #             with mock.patch('os.path.isdir', return_value=False), \
    #                  mock.patch('os.path.isfile', return_value=True), \
    #                  mock.patch(NAMESPACE + '.print_info'):
    #                 key = JobDispatcher('123')
    #                 key.add_symlink()
    #             with mock.patch('os.path.isdir', return_value=False), \
    #                  mock.patch('os.path.isfile', return_value=False), \
    #                  mock.patch(NAMESPACE + '.check_path_valid'), \
    #                  mock.patch('os.symlink'):
    #                 key = JobDispatcher('123')
    #                 key.add_symlink()
    #         with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', side_effect=OSError), \
    #              mock.patch(NAMESPACE + '.logging.error'):
    #             key = JobDispatcher('123')
    #             key.add_symlink()

    def test_clean_invalid_dispatch_symlink(self):
        with mock.patch(NAMESPACE + '.PathManager.get_dispatch_dir', return_value=True), \
             mock.patch('os.path.realpath', return_value=True):
            with mock.patch('os.path.exists', return_value=False):
                key = JobDispatcher('123')
                key._clean_invalid_dispatch_symlink()
            with mock.patch('os.path.exists', return_value=True), \
                 mock.patch('os.listdir', return_value=[1, 2]):
                with mock.patch('os.path.join', return_value='123'), \
                     mock.patch('os.path.isdir', return_value=False), \
                     mock.patch('os.path.islink', return_value=True), \
                     mock.patch('os.remove'):
                    key = JobDispatcher('123')
                    key._clean_invalid_dispatch_symlink()
                with mock.patch('os.path.isdir', side_effect=OSError), \
                     mock.patch('os.path.join', return_value='123'), \
                     mock.patch(NAMESPACE + '.logging.error'):
                    key = JobDispatcher('123')
                    key._clean_invalid_dispatch_symlink()

    def test_process(self):
        with mock.patch(NAMESPACE + '.print_info', return_value=True):
            with mock.patch(NAMESPACE + '.check_path_valid', return_value=True), \
                 mock.patch(NAMESPACE + '.PathManager.get_dispatch_dir', return_value=True), \
                 mock.patch(NAMESPACE + '.JobDispatcher._clean_invalid_dispatch_symlink'), \
                 mock.patch(NAMESPACE + '.JobDispatcher.add_symlink', side_effect=OSError), \
                 mock.patch('time.sleep', side_effect=KeyboardInterrupt), \
                 mock.patch(NAMESPACE + '.logging.error'), \
                 pytest.raises(SystemExit) as err:
                key = JobDispatcher('123')
                key.process()
            self.assertEqual(err.value.code, 0)

    def test_analyze_data(self):
        with mock.patch('os.listdir', return_value=['123', '456']), \
             mock.patch('os.path.realpath', return_value='home\\123'), \
             mock.patch('os.path.join', return_value=True), \
             mock.patch('os.path.basename', return_value=['321', '654']), \
             mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
             mock.patch(NAMESPACE + '.LoadInfoManager.load_info'), \
             mock.patch(NAMESPACE + '.JobMonitor._analysis_job_profiling', return_value=True):
            InfoConfReader()._info_json = {"DeviceInfo": 'home\\123'}
            key = JobMonitor('123')
            key.analyze_data()
        with mock.patch('os.listdir', side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'):
            key = JobMonitor('123')
            key.analyze_data()

    def test_process(self):
        with mock.patch(NAMESPACE + '.print_info', return_value=True):
            with mock.patch('os.path.exists', return_value=True), \
                 mock.patch(NAMESPACE + '.JobMonitor.analyze_data'), \
                 mock.patch('time.sleep', side_effect=KeyboardInterrupt), \
                 pytest.raises(SystemExit) as err:
                key = JobMonitor('123')
                key.process()
            self.assertEqual(err.value.code, 0)

    def test_analysis_job_profiling(self):
        job_path = 'home\\job'
        job_tag = 'job_files'
        with mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='sample'), \
             mock.patch(NAMESPACE + '.LogFactory.get_logger', return_value=123):
            with mock.patch('os.path.exists', return_value=False), \
                 mock.patch(NAMESPACE + '.logging.error'):
                key = JobMonitor('123')
                key._analysis_job_profiling(job_path, job_tag)
            with mock.patch('os.path.exists', return_value=True), \
                 mock.patch(NAMESPACE + '.FileManager.is_analyzed_data', return_value=False), \
                 mock.patch('common_func.msvp_common.PathManager.get_sql_dir', return_value='test'), \
                 mock.patch('os.path.exists', return_value=True), \
                 mock.patch('os.listdir', return_value=['test.complete']), \
                 mock.patch('os.path.join', return_value='test/test'), \
                 mock.patch('os.remove', return_value=True), \
                 mock.patch(NAMESPACE + '.generate_config', return_value={'device_info': 1}), \
                 mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=True), \
                 mock.patch(NAMESPACE + '.check_path_valid'), \
                 mock.patch(NAMESPACE + '.JobMonitor._launch_parsing_job_data'):
                key = JobMonitor('123')
                key._analysis_job_profiling(job_path, job_tag)


def test_monitor_job():
    num = 123
    collection_path = 'home\\collection'
    with mock.patch('os.path.relpath', return_value=True), \
         mock.patch(NAMESPACE + '.PathManager.get_dispatch_dir', return_value='home\\dispatch'), \
         mock.patch('os.path.join', return_value='home\\monitor'), \
         mock.patch(NAMESPACE + '.JobMonitor.process'):
        _monitor_job(num, collection_path)


def test_dispatch_job():
    collection_path = 'home\\collection'
    with mock.patch(NAMESPACE + '.JobDispatcher.process'):
        _dispatch_job(collection_path)


def test_monitor():
    collection_path = 'home\\collection'
    with mock.patch(NAMESPACE + '.check_path_valid', return_value=True), \
         mock.patch('multiprocessing.Process.start'), \
         mock.patch('multiprocessing.Process.join'):
        monitor(collection_path)
    with mock.patch(NAMESPACE + '.check_path_valid', return_value=True), \
         mock.patch('multiprocessing.Process.start', side_effect=KeyboardInterrupt), \
         mock.patch(NAMESPACE + '.print_info'):
        monitor(collection_path)
    with mock.patch('sys.exit'), \
         mock.patch('os.listdir', return_value=['test.complete']):
        with mock.patch(NAMESPACE + '.check_path_valid', return_value=True), \
             mock.patch('multiprocessing.Process.start'), \
             mock.patch('multiprocessing.Process.join'):
            monitor(collection_path)
    dir_list = (['test_0', 'test_1'], ['.profiler'], ['sqlite'], ['test_0', 'test_1'], ['profiler'], ['sqlite1'])
    with mock.patch(NAMESPACE + '.check_path_valid', return_value=True), \
         mock.patch('os.listdir', side_effect=dir_list), \
         mock.patch('common_func.msprof_common.PathManager.get_data_dir', return_value=['test_0']), \
         mock.patch('multiprocessing.Process.start'), \
         mock.patch('multiprocessing.Process.join'):
        monitor(collection_path)


if __name__ == '__main__':
    unittest.main()
