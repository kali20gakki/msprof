import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.stars_constant import StarsConstant
from msmodel.stars.stars_chip_trans_model import StarsChipTransModel
from profiling_bean.stars.stars_chip_trans_bean import StarsChipTransBean

NAMESPACE = 'msmodel.stars.stars_chip_trans_model'


class TestStarsChipTransModel(unittest.TestCase):

    def test_flush(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        data_dict = {StarsConstant.TYPE_STARS_PA: [], "01": []}
        with mock.patch(NAMESPACE + '.StarsChipTransModel.insert_data_to_db'):
            check = StarsChipTransModel('test', 'test', [])
            check.flush(data_dict)
