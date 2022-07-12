import unittest
from unittest import mock

from msmodel.ge.ge_info_model import GeModel

NAMESPACE = 'msmodel.ge.ge_info_model'


class TestGeHashModel(unittest.TestCase):
    def test_flush(self):
        with mock.patch(NAMESPACE + '.GeModel.insert_data_to_db'):
            GeModel('test', ['test']).flush([])

    def test_get_ge_hash_model_name(self):
        GeModel('test', ['test']).get_ge_model_name()

    def test_flush_all(self):
        with mock.patch(NAMESPACE + '.GeModel.flush'):
            GeModel('test', ['test']).flush_all({'test': 'test'})
