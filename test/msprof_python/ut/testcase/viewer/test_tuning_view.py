import json
import unittest
from unittest import mock
from viewer.tuning_view import TuningView

NAMESPACE = 'viewer.tuning_view'


class TestTuningView(unittest.TestCase):
    def test_show_by_dev_id(self):
        with mock.patch(NAMESPACE + '.TuningView.tuning_report'):
            TuningView("", {}).show_by_dev_id(0)

    def test_load_result_file_1(self):
        read = json.dumps({"data": "", "status": 1})
        with mock.patch('os.path.exists', side_effect=FileNotFoundError):
            res = TuningView("", {})._load_result_file(0)
        self.assertEqual(res, {})

        with mock.patch('os.path.exists', return_value=False):
            res = TuningView("", {})._load_result_file(0)
        self.assertEqual(res, {})

        with mock.patch('os.path.exists', return_value=True), \
                mock.patch('builtins.open', mock.mock_open(read_data=read)):
            res = TuningView("", {})._load_result_file(0)
        self.assertEqual(res, {'data': '', 'status': 1})

    def test_tuning_report(self):
        data = {"data": [{"result": 1}], "rule_type": 1}
        with mock.patch(NAMESPACE + '.TuningView._load_result_file', return_value=None):
            TuningView("", {}).tuning_report(0)

        with mock.patch(NAMESPACE + '.TuningView._load_result_file', return_value=data), \
                mock.patch(NAMESPACE + '.TuningView.print_first_level'), \
                mock.patch(NAMESPACE + '.TuningView.print_second_level'):
            TuningView("", {}).tuning_report(0)

    def test_print_first_level(self):
        data = {"rule_type": 1}
        TuningView("", {}).print_first_level(0, data)

    def test_print_second_level(self):
        data = [{"rule_type": "1", "op_list": "3", "rule_suggestion": "4"}]
        data_sub = [{"rule_type": "1", "rule_subtype": "2", "op_list": "3", "rule_suggestion": "4"},
                    {"rule_type": "1", "rule_subtype": "2", "op_list": "3", "rule_suggestion": "4"}]
        TuningView("", {}).print_second_level(None)
        TuningView("", {}).print_second_level(data)
        TuningView("", {}).print_second_level(data_sub)


if __name__ == '__main__':
    unittest.main()
