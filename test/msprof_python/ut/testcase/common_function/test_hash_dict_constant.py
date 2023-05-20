import unittest
from unittest import mock

from common_func.hash_dict_constant import HashDictData

NAMESPACE = 'common_func.file_manager'


class TestFileManager(unittest.TestCase):

    def test_load_hash_data(self):
        with mock.patch('common_func.singleton.singleton'), \
                mock.patch('msmodel.ge.ge_hash_model.GeHashViewModel.get_type_hash_data',
                           return_value={'acl': {'1': 'test'}}), \
                mock.patch('msmodel.ge.ge_hash_model.GeHashViewModel.get_ge_hash_data',
                           return_value={'123': 'test_ge_hash'}):
            check = HashDictData('test')
            self.assertEqual(check.get_type_hash_dict(), {'acl': {'1': 'test'}})
            self.assertEqual(check.get_ge_hash_dict(), {'123': 'test_ge_hash'})
