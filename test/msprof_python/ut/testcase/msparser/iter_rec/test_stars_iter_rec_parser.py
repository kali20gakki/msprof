import sqlite3
import struct
import unittest
from unittest import mock

import pytest

from common_func.msprof_exception import ProfException
from msparser.iter_rec.stars_iter_rec_parser import StarsIterRecParser
from msparser.iter_rec.iter_info_updater.iter_info import IterInfo
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.ffts_pmu import FftsPmuBean
from profiling_bean.db_dto.step_trace_dto import StepTraceDto

NAMESPACE = 'msparser.iter_rec.stars_iter_rec_parser'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


def get_step_trace_data():
    step_trace_dto = StepTraceDto()
    step_trace_dto.step_start = 1
    step_trace_dto.step_end = 3
    step_trace_dto.iter_id = 1
    return step_trace_dto


class TestStarsIterRecParser(unittest.TestCase):
    stars_file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0']}
    ffts_file_list = {DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}

    def test_ms_run(self):
        with mock.patch('common_func.utils.Utils.get_scene', return_value='step_trace'), \
                mock.patch(NAMESPACE + '.StarsIterRecParser.parse'), \
                mock.patch(NAMESPACE + '.StarsIterRecParser.save'):
            StarsIterRecParser(self.stars_file_list, sample_config).ms_run()
        with mock.patch('common_func.utils.Utils.get_scene', return_value='step_trace'), \
                mock.patch(NAMESPACE + '.StarsIterRecParser.parse'), \
                mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch(NAMESPACE + '.StarsIterRecParser.save', side_effect=ProfException(8)):
            StarsIterRecParser(self.stars_file_list, sample_config).ms_run()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.StarsIterRecParser._get_iter_end_dict'), \
                mock.patch(NAMESPACE + '.StarsIterRecParser._parse_pmu_data'), \
                mock.patch(NAMESPACE + '.StarsIterRecParser._parse_log_file_list'):
            StarsIterRecParser(self.stars_file_list, sample_config).parse()

    def test_save(self):
        with mock.patch(NAMESPACE + '.HwtsIterModel.flush'), \
                mock.patch(NAMESPACE + '.GeInfoModel.get_step_trace_data', return_value=[get_step_trace_data()]):
            check = StarsIterRecParser(self.stars_file_list, sample_config)
            iter_info = IterInfo(1, 1, 1, 2, 3)
            check._iter_info_dict = {1: iter_info}
            check._task_cnt_not_in_iter = {1: 2}
            check.save()
        with mock.patch(NAMESPACE + '.HwtsIterModel.flush', side_effect=sqlite3.Error), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = StarsIterRecParser(self.stars_file_list, sample_config)
            check._iter_info_dict = {100: FftsPmuBean}
            check.save()

    def test_parse_log_file_list(self):
        data = struct.pack("=HHLQ12L", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch('os.path.getsize', return_value=64), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch(NAMESPACE + '.StarsIterRecParser._process_log_data'):
            StarsIterRecParser(self.stars_file_list, sample_config)._parse_log_file_list()

    def test_process_log_data(self):
        data = b'0' * 64
        with mock.patch(NAMESPACE + '.StarsIterRecParser._set_current_iter_id'), \
                mock.patch(NAMESPACE + '.StarsIterRecParser._update_iter_info'), \
                mock.patch(NAMESPACE + '.IterRecorder.check_task_in_iter', return_value=False):
            StarsIterRecParser(self.stars_file_list, sample_config)._process_log_data(data)

        with mock.patch(NAMESPACE + '.StarsIterRecParser._set_current_iter_id'), \
                mock.patch(NAMESPACE + '.StarsIterRecParser._update_iter_info'), \
                mock.patch(NAMESPACE + '.IterRecorder.check_task_in_iter', return_value=True):
            StarsIterRecParser(self.stars_file_list, sample_config)._process_log_data(data)

    def test_parse_pmu_data(self):
        data = struct.pack("=4HQ4HQ12Q", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch('os.path.getsize', return_value=128), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch(NAMESPACE + '.StarsIterRecParser._process_pmu_data'):
            StarsIterRecParser(self.ffts_file_list, sample_config)._parse_pmu_data()

    def test_set_current_iter_id(self):
        check = StarsIterRecParser(self.ffts_file_list, sample_config)
        check._iter_end_dict = {100: 200}
        check._set_current_iter_id(100)
        with mock.patch(NAMESPACE + '.logging.error'), \
                pytest.raises(ProfException) as error:
            check = StarsIterRecParser(self.ffts_file_list, sample_config)
            check._iter_end_dict = {}
            check._set_current_iter_id(100)
            self.assertEqual(error, 8)

    def test_process_pmu_data(self):
        data = b'0' * 128
        with mock.patch(NAMESPACE + '.StarsIterRecParser._set_current_iter_id'), \
                mock.patch(NAMESPACE + '.StarsIterRecParser._update_iter_info'):
            StarsIterRecParser(self.stars_file_list, sample_config)._process_pmu_data(data)

    def test_check_current_iter_id(self):
        check = StarsIterRecParser(self.ffts_file_list, sample_config)
        check._iter_end_dict = {100: 200}
        check._check_current_iter_id(100)

    def test_update_iter_info(self):
        with mock.patch(NAMESPACE + '.StarsIterRecParser._check_current_iter_id', side_effect=(True, False)):
            check = StarsIterRecParser(self.ffts_file_list, sample_config)
            check._iter_end_dict = {100: 200}
            check._update_iter_info(100, 200)

    def test_get_iter_end_dict(self):
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_end_dict',
                        return_value={}), \
                mock.patch(NAMESPACE + '.logging.info'):
            with pytest.raises(ProfException) as error:
                StarsIterRecParser(self.stars_file_list, sample_config)._get_iter_end_dict()
                self.assertEqual(error.value, 8)
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_end_dict',
                        return_value={1: 2}):
            self.assertEqual(StarsIterRecParser(self.stars_file_list, sample_config)._get_iter_end_dict(), {1: 2})
