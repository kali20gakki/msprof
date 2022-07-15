import struct
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from profiling_bean.prof_enum.data_tag import DataTag

from msparser.stars.stars_log_parser import StarsLogCalCulator
from msparser.stars.parser_dispatcher import ParserDispatcher

NAMESPACE = 'msparser.stars.stars_log_parser'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestStarsLogCalCulator(unittest.TestCase):

    def test_init_dispatcher(self):
        with mock.patch('msparser.stars.parser_dispatcher.ParserDispatcher.init'):
            check = StarsLogCalCulator({}, sample_config)
            check.init_dispatcher()

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.StarsLogCalCulator.init_dispatcher'), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='123'), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='step_info'), \
                mock.patch(NAMESPACE + '.StarsLogCalCulator._parse_by_iter'):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2']}, sample_config={'1': 'ada'})
            key.calculate()
        ProfilingScene().init(sample_config.get('result_dir'))
        with mock.patch(NAMESPACE + '.StarsLogCalCulator.init_dispatcher'), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='123'), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='single_op'), \
                mock.patch(NAMESPACE + '.StarsLogCalCulator._parse_all_file'):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2']}, sample_config={'1': 'ada'})
            key.calculate()

    def test_save(self):
        with mock.patch('msparser.stars.parser_dispatcher.ParserDispatcher.flush_all_parser'):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']}, sample_config={'1': 'ada'})
            key._parser_dispatcher = ParserDispatcher(result_dir='11')
            key.save()

    def test_ms_run(self):
        key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']}, sample_config={'1': 'ada'})
        with mock.patch(NAMESPACE + '.StarsLogCalCulator.calculate'), \
                mock.patch(NAMESPACE + '.StarsLogCalCulator.save'):
            key.ms_run()

    # def test_parse_all_file(self): XXX
    #     data = struct.pack('=4HQ4HQ12Q', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    #     with mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
    #             mock.patch('builtins.open', mock.mock_open(read_data=data)), \
    #             mock.patch('os.path.getsize', return_value=128):
    #         with mock.patch('msparser.stars.parser_dispatcher.ParserDispatcher.flush_all_parser'):
    #             key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']}, sample_config={'1': 'ada'})
    #             key._parser_dispatcher = ParserDispatcher(result_dir='11')
    #             key._parse_all_file()

    def test_parse_by_iter(self):
        with mock.patch('msmodel.interface.parser_model.ParserModel.init'), \
             mock.patch(NAMESPACE + '.HwtsIterModel.get_task_offset_and_sum', return_value=(0, 0)):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']}, sample_config={'1': 'ada'})
            key._parser_dispatcher = ParserDispatcher(result_dir='11')
            key._parse_by_iter()
        with mock.patch('msmodel.interface.parser_model.ParserModel.init'), \
             mock.patch(NAMESPACE + '.HwtsIterModel.get_task_offset_and_sum', return_value=(63, 63)), \
                mock.patch(NAMESPACE + '.FileCalculator.prepare_process', return_value=(63, 63)):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']}, sample_config={'1': 'ada'})
            key._parser_dispatcher = ParserDispatcher(result_dir='11')
            key._parse_by_iter()


if __name__ == '__main__':
    unittest.main()
