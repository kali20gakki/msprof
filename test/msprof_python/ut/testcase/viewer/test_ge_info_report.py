import sqlite3
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from viewer.ge_info_report import get_ge_model_data


NAMESPACE = 'viewer.ge_info_report'
configs = {'handler': '_get_acl_data',
           'headers': ['model_name', 'model_id', 'device_id'],
           'db': 'acl_module.db',
           'table': 'acl_data', 'unused_cols': [],
           'columns': "modelname, model_id, device_id"}


class TestGeInfoReport(unittest.TestCase):

    def test_get_ge_model_data_1(self):
        with mock.patch(NAMESPACE + '.ViewModel.check_table', return_value=False):
            res = get_ge_model_data({}, DBNameConstant.TABLE_GE_FUSION_OP_INFO, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.ViewModel.check_table', return_value=True), \
             mock.patch(NAMESPACE + '.ViewModel.get_sql_data', return_value=[]):
            res = get_ge_model_data({}, DBNameConstant.TABLE_GE_FUSION_OP_INFO, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_ge_model_data_2(self):
        data = [('ge_default_20211216143901_61', 5, 'one_hot', '16883296179185069540',
                 288.0, 128160.0, 0.0, 0.0, 128448.0)]

        with mock.patch(NAMESPACE + '.ViewModel.check_table', return_value=True), \
             mock.patch(NAMESPACE + '.get_ge_hash_dict', return_value={}), \
             mock.patch('msmodel.interface.view_model.ViewModel.get_sql_data', return_value=data):
            res = get_ge_model_data({}, DBNameConstant.TABLE_GE_FUSION_OP_INFO, configs)
        self.assertEqual(res[2], 1)


if __name__ == '__main__':
    unittest.main()
