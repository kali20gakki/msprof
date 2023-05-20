import unittest
from unittest import mock

from msmodel.add_info.fusion_add_info_model import FusionAddInfoModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.add_info.fusion_add_info_model'


class TestFusionAddInfoModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.FusionAddInfoModel.insert_data_to_db'):
            self.assertEqual(FusionAddInfoModel('test').flush([]), None)


if __name__ == '__main__':
    unittest.main()
