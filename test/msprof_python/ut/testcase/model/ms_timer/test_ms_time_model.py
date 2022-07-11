import unittest
from unittest import mock

from msmodel.ms_timer.ms_time_model import MsTimeModel

NAMESPACE = "msmodel.ms_timer.ms_time_model"


class TsetMsTimeModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.MsTimeModel.insert_data_to_db'):
            check = MsTimeModel("test")
            check.flush([])