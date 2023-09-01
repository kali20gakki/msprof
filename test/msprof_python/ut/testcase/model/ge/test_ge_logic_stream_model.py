import unittest
from unittest import mock

from msmodel.ge.ge_logic_stream_model import GeLogicStreamInfoModel

NAMESPACE = 'msmodel.ge.ge_logic_stream_model'


class TestGeLogicStreamModel(unittest.TestCase):
    def test_flush_should_success_when_model_created(self):
        with mock.patch(NAMESPACE + '.GeLogicStreamInfoModel.insert_data_to_db'):
            GeLogicStreamInfoModel('test')
