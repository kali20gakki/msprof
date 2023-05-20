import unittest
from unittest import mock

from msmodel.add_info.memory_application_model import MemoryApplicationModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.add_info.memory_application_model'


class TestCtxIdModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.MemoryApplicationModel.insert_data_to_db'):
            self.assertEqual(MemoryApplicationModel('test').flush([]), None)


if __name__ == '__main__':
    unittest.main()
