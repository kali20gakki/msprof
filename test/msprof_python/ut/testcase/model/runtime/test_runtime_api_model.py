import unittest
from unittest import mock

from msmodel.runtime.runtime_api_model import RuntimeApiModel


NAMESPACE = 'msmodel.runtime.runtime_api_model'


class TestMsprofTxModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.RuntimeApiModel.insert_data_to_db'):
            check = RuntimeApiModel('test', 'runtime.db', ['ApiCall'])
            check.flush([])

    def test_class_name(self):
        with mock.patch(NAMESPACE + '.RuntimeApiModel.init'),\
                mock.patch(NAMESPACE + '.RuntimeApiModel.check_db'),\
                mock.patch(NAMESPACE + '.RuntimeApiModel.create_table'),\
                mock.patch(NAMESPACE + '.RuntimeApiModel.finalize'),\
                mock.patch(NAMESPACE + '.RuntimeApiModel.insert_data_to_db'):
            check = RuntimeApiModel('test', 'runtime.db', ['RuntimeTrack'])
            check.class_name()
