import unittest
from common_func.msvp_common import format_high_precision_for_csv


def test_format_high_precision_for_csv_and_should_add_tab():
    data = '12345678912345.2345324'
    res = format_high_precision_for_csv(data)
    unittest.TestCase().assertEqual(res, '12345678912345.2345324\t')


if __name__ == '__main__':
    unittest.main()
