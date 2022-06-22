import unittest
from unittest import mock
from msmodel.interface.parser_model import ParserModel

NAMESPACE = 'msmodel.interface.parser_model'


class DemoClass(ParserModel):
    def flush(self: any, data_list: list):
        pass


class TestBaseModel(unittest.TestCase):

    def test_init0(self):
        expect_res = True
        check = DemoClass('test', 'test', 'test')
        with mock.patch(NAMESPACE + '.ParserModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ParserModel.create_table'):
            res = check.init()
        self.assertEqual(expect_res, res)

    def test_init1(self):
        expect_res = False
        check = DemoClass('test', 'test', 'test')
        with mock.patch(NAMESPACE + '.ParserModel.init', return_value=False):
            res = check.init()
        self.assertEqual(expect_res, res)
