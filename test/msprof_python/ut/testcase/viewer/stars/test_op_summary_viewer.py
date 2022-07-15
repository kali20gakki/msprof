import sqlite3
import unittest
from unittest import mock

from viewer.stars.op_summary_viewer import OpSummaryViewer

NAMESPACE = 'viewer.stars.op_summary_viewer'


class TestOpSummaryViewer(unittest.TestCase):

    def test_get_summary_data_with_exception(self):
        with mock.patch(NAMESPACE + '.OpSummaryModel', side_effect=sqlite3.DatabaseError):
            check = OpSummaryViewer({}, {})
            ret = check.get_summary_data()
            self.assertEqual([], ret)

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + '.OpSummaryModel.get_summary_data', return_value=[]),\
             mock.patch('msmodel.interface.view_model.ViewModel.init'):
            check = OpSummaryViewer({}, {})
            ret = check.get_summary_data()
            self.assertEqual([], ret)
