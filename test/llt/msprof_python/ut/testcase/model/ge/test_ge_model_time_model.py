
import unittest
from unittest import mock

from model.ge.ge_model_time_load import GeModelTimeModel

NAMESPACE = 'model.ge.ge_model_time_load'


class TestGeHashModel(unittest.TestCase):
    def test_flush(self):
        with mock.patch(NAMESPACE + '.GeModelTimeModel.insert_data_to_db'):
            GeModelTimeModel('test', ['test']).flush([])

    def test_get_ge_hash_model_name(self):
        GeModelTimeModel('test', ['test']).get_ge_model_time_model_name()
