import unittest
from unittest import mock

from model.ge.ge_model_load_model import GeFusionModel

NAMESPACE = 'model.ge.ge_model_load_model'


class TestGeHashModel(unittest.TestCase):
    def test_flush(self):
        with mock.patch(NAMESPACE + '.GeFusionModel.insert_data_to_db'):
            GeFusionModel('test', ['test']).flush([])

    def test_get_ge_hash_model_name(self):
        GeFusionModel('test', ['test']).get_ge_fusion_model_name()

    def test_flush_all(self):
        with mock.patch(NAMESPACE + '.GeFusionModel.flush'):
            GeFusionModel('test', ['test']).flush_all({'test': 'test'})
