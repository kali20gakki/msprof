import unittest
from unittest import mock

from msmodel.msproftx.msproftx_model import MsprofTxModel


NAMESPACE = 'msmodel.msproftx.msproftx_model'


class TestMsprofTxModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.MsprofTxModel.insert_data_to_db'):
            check = MsprofTxModel('test', 'msproftx.db', ['MsprofTx'])
            check.flush([])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = MsprofTxModel('test', 'msproftx.db', ['MsprofTx'])
            check.get_timeline_data()
