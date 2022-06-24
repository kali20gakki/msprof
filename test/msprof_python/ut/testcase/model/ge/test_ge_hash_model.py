import unittest
from unittest import mock

from msmodel.ge.ge_hash_model import GeHashModel

NAMESPACE = 'msmodel.ge.ge_hash_model'


class TestGeHashModel(unittest.TestCase):
    def test_flush(self):
        with mock.patch(NAMESPACE + '.GeHashModel.insert_data_to_db'):
            GeHashModel('test', ['test']).flush([])

    def test_get_ge_hash_model_name(self):
        GeHashModel('test', ['test']).get_ge_hash_model_name()
