import json
import unittest
from unittest import mock
from viewer.tuning_view import TuningView

NAMESPACE = 'viewer.tuning_view'


class TestTuningView(unittest.TestCase):
    def test_show_by_dev_id(self):
        with mock.patch(NAMESPACE + '.TuningView.tuning_report'):
            TuningView("", {}, 0).show_by_dev_id()

    def test_load_result_file_1(self):
        with mock.patch('builtins.open', side_effect=FileNotFoundError), \
                mock.patch(NAMESPACE + '.PathManager.get_summary_dir', return_value='test'), \
                mock.patch('os.path.exists', return_value=True):
            res = TuningView("", {}, 0).get_tuning_data()
        self.assertEqual(res, None)

    def test_tuning_report(self):
        data = {"data": [{"result": [{'rule_subtype': 'test'}]}], "rule_type": 1}
        with mock.patch(NAMESPACE + '.PathManager.get_summary_dir', return_value='test'),\
                mock.patch('os.path.exists', return_value=True),\
                mock.patch('builtins.open', mock.mock_open(read_data=json.dumps(data))):

            TuningView("", {}, 0).tuning_report()

        with mock.patch(NAMESPACE + '.TuningView._load_result_file', return_value=data), \
                mock.patch(NAMESPACE + '.TuningView.print_first_level'), \
                mock.patch(NAMESPACE + '.TuningView.print_second_level'):
            TuningView("", {}, 0).tuning_report()

    def test_print_first_level(self):
        data = {"rule_type": 1}
        TuningView("", {}, 0).print_first_level(0, data)

    def test_print_second_level(self):
        data = [{"rule_type": "1", "op_list": "3", "rule_suggestion": "4"}]
        data_sub = [{"rule_type": "1", "rule_subtype": "2", "op_list": "3", "rule_suggestion": "4"},
                    {"rule_type": "1", "rule_subtype": "2", "op_list": "3", "rule_suggestion": "4"}]
        TuningView("", {}, 0).print_second_level(None)
        TuningView("", {}, 0).print_second_level(data)
        TuningView("", {}, 0).print_second_level(data_sub)


if __name__ == '__main__':
    unittest.main()
