import configparser
import unittest
from unittest import mock

from msparser.stars.parser_dispatcher import ParserDispatcher
from msparser.interface.istars_parser import IStarsParser

NAMESPACE = 'msparser.stars.parser_dispatcher'


class TestParserDispatcher(unittest.TestCase):
    def test_init(self):
        result_dir = 'ada.py'
        with mock.patch(NAMESPACE + '.ParserDispatcher.build_parser_map'):
            key = ParserDispatcher(result_dir)
            key.init()

    def test_dispatch_1(self):
        func_type = 'is'
        data = '123'
        with mock.patch('msparser.interface.istars_parser.IStarsParser.handle'):
            key = ParserDispatcher(result_dir='ada')
            key.parser_map = {'is': IStarsParser}
            key.dispatch(func_type, data)

    def test_dispatch_2(self):
        func_type = 'self'
        data = '111'
        with mock.patch(NAMESPACE + '.logging.error'):
            key = ParserDispatcher(result_dir='ada')
            key.dispatch(func_type, data)

    def test_flush_all_parser(self):
        with mock.patch('msparser.interface.istars_parser.IStarsParser.flush'):
            key = ParserDispatcher(result_dir='ada')
            key.parser_map = {'fftspmuparser': IStarsParser}
            key.flush_all_parser()

    def test_build_parser_map(self):
        with mock.patch('configparser.ConfigParser.sections', return_value=['AsqTaskParser']), \
                mock.patch('configparser.ConfigParser.get', return_value='1'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'):
            check = ParserDispatcher('test')
            check.cfg_parser = configparser.ConfigParser(interpolation=None)
            check.build_parser_map()


if __name__ == '__main__':
    unittest.main()

