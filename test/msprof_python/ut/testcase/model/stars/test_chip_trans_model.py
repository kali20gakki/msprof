import unittest
from unittest import mock

from common_func.ms_constant.stars_constant import StarsConstant
from model.stars.stars_chip_trans_model import StarsChipTransModel

NAMESPACE = 'model.stars.stars_chip_trans_model'


class TestStarsChipTransModel(unittest.TestCase):

    def test_flush(self):
        data_dict = {StarsConstant.TYPE_STARS_PA: [], "01": []}
        with mock.patch(NAMESPACE + '.StarsChipTransModel.insert_data_to_db'):
            check = StarsChipTransModel('test', 'test', [])
            check.flush(data_dict)
