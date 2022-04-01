import unittest

import pytest
from viewer.calculate_rts_data import check_aicore_events
from viewer.calculate_rts_data_training import get_task_type

NAMESPACE = 'viewer.calculate_rts_data_training'


class TestCalculateRtsTrain(unittest.TestCase):
    def test_check_aicore_events(self):
        events = ["0x8", "0xa", "0x9", "0xb", "0xc", "0xd", "0x54", "0x55", 0x1]
        check_aicore_events(events)

        with pytest.raises(SystemExit) as error:
            check_aicore_events([])
        self.assertEqual(error.value.args[0], 1)

    def test_get_task_type(self):
        res = get_task_type("1")
        self.assertEqual(res, 'kernel AI cpu task')

        res = get_task_type("")
        self.assertEqual(res, "")


if __name__ == '__main__':
    unittest.main()
