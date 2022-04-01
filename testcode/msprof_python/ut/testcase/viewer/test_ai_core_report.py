import unittest
from unittest import mock
from common_func.msvp_constant import MsvpConstant
from viewer.ai_core_report import get_core_sample_data, get_output_event_counter, _get_output_event_counter

NAMESPACE = 'viewer.ai_core_report'
params = {'core_data_type': 'ai_core_pmu_events',
          'project': '', 'device_id': '0',
          'job_id': 'job_default',
          'export_type': 'summary', 'iter_id': 1,
          'export_format': 'csv', 'model_id': 1}


class TestAclReport(unittest.TestCase):
    def test_get_core_sample_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = get_core_sample_data('', '', 0, params)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_core_sample_data_2(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.get_output_event_counter', return_value=([], [])):
            res = get_core_sample_data('', '', 0, params)
        self.assertEqual(res[1], [])

    def test_get_core_sample_data_3(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.get_output_event_counter', return_value=([], [])):
            params["data_type"] = "ai_vector_core_pmu_events"
            res = get_core_sample_data('', '', 0, params)
        self.assertEqual(res[1], [])

    def test_get_core_sample_data_4(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.get_output_event_counter', return_value=([], [])):
            params["data_type"] = ""
            res = get_core_sample_data('', '', 0, params)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_output_event_counter_1(self):
        res = get_output_event_counter(None)
        self.assertEqual(res, ([], []))

    def test_get_output_event_counter_2(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_output_event_counter('')
        self.assertEqual(res, ([], []))

    def test_get_output_event_counter_3(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            res = get_output_event_counter('')
        self.assertEqual(res, ([], []))

    def test_get_output_event_counter_4(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]), \
             mock.patch(NAMESPACE + '._get_output_event_counter', return_value=([], [])):
            res = get_output_event_counter('')
        self.assertEqual(res, ([], []))

    def test__get_output_event_counter_1(self):
        res = _get_output_event_counter([["vec_bankgroup_cflt_ratio", 0], ["vec_bankgroup_cflt_ratio", 1]], [(0,), (1,)])
        self.assertEqual(res, (["Core ID", "vec_bankgroup_cflt_ratio"], [["Core0", 0.0], ["Core1", 1.0], ["Average", 0.5]]))


if __name__ == '__main__':
    unittest.main()
