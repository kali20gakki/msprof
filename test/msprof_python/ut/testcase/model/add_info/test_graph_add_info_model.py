import unittest
from unittest import mock

from msmodel.add_info.graph_add_info_model import GraphAddInfoModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.add_info.graph_add_info_model'


class TestGraphAddInfoModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.GraphAddInfoModel.insert_data_to_db'):
            self.assertEqual(GraphAddInfoModel('test').flush([]), None)


if __name__ == '__main__':
    unittest.main()
