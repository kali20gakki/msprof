import unittest
from unittest import mock

from msmodel.add_info.tensor_add_info_model import TensorAddInfoModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.add_info.tensor_add_info_model'


class TestCtxIdModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.TensorAddInfoModel.insert_data_to_db'):
            self.assertEqual(TensorAddInfoModel('test').flush([]), None)


if __name__ == '__main__':
    unittest.main()
