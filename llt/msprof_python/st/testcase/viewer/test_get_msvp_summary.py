import json
import sqlite3
import unittest
from unittest import mock

from common_func.ms_constant.number_constant import NumberConstant
from sqlite.db_manager import DBOpen
from viewer.get_msvp_summary import get_aicore_utilization
from viewer.get_msvp_summary import get_type_db_correspondences
from viewer.get_msvp_summary import pre_check_pmu_events_interface

from sqlite.db_manager import DBManager

NAMESPACE = 'viewer.get_msvp_summary'
param = {'project': "", "project_path": '', "device_id": 0,
         "start_time": 0, "end_time": 9223372036854775807,
         "data_type": "llc_aicpu"}
sample_config = {"ai_core_profiling_mode": "bandwidth"}
info_json = {"DeviceInfo": [
    {"id": 0, "env_type": 3, "ctrl_cpu_id": "ARMv8_Cortex_A55",
     "ctrl_cpu_core_num": 1, "ctrl_cpu_endian_little": 1,
     "ts_cpu_core_num": 4, "ai_cpu_core_num": 14, "ai_core_num": 32,
     "ai_cpu_core_id": 2, "ai_core_id": 0,
     "aicpu_occupy_bitmap": 65532, "ctrl_cpu": "0",
     "ai_cpu": "2,3,4,5,6,7,8,9,10,11,12,13,14",
     "devFrequency": "2.7GHz", "aiv_num": 0, "hwts_frequency": "100",
     "ai_core_profiling_mode": "sample-based",
     "aic_frequency": "1000", "aiv_frequency": "1000"}]}


class TestMsvpSummary(unittest.TestCase):

    def test_get_type_db_correspondences(self):
        sample_config_test = {"ai_core_profiling_mode": "task-based"}
        res = get_type_db_correspondences(0, "type", sample_config_test)
        self.assertEqual(len(res), 3)

        res = get_type_db_correspondences(0, "ai_core_profiling", sample_config_test)
        self.assertEqual(len(res), 1)

    def test_pre_check_pmu_events_interface(self):
        with mock.patch(NAMESPACE + '.path_check', side_effect=OSError):
            res = pre_check_pmu_events_interface("a", 0, "ai_core_profiling")
        self.assertEqual(res[0], NumberConstant.ERROR)

        with mock.patch(NAMESPACE + '.path_check', return_value=None):
            res = pre_check_pmu_events_interface("", 0, "ai_core_profiling")
        self.assertEqual(res[0], NumberConstant.ERROR)

        with mock.patch(NAMESPACE + '.path_check', return_value="a"), \
             mock.patch(NAMESPACE + '.generate_config', return_value=None):
            res = pre_check_pmu_events_interface("", 0, "ai_core_profiling")
        self.assertEqual(res[0], NumberConstant.ERROR)

        with mock.patch(NAMESPACE + '.path_check', return_value="a"), \
             mock.patch(NAMESPACE + '.generate_config', return_value=info_json):
            res = pre_check_pmu_events_interface("", 0, "ai_core_profiling")
        self.assertEqual(res[0], NumberConstant.SUCCESS)

        with mock.patch(NAMESPACE + '.path_check', return_value="a"), \
             mock.patch(NAMESPACE + '.generate_config', return_value=info_json), \
             mock.patch(NAMESPACE + '.get_type_db_correspondences', return_value=None):
            res = pre_check_pmu_events_interface("", 0, "ai_core_profiling")
        self.assertEqual(res[0], NumberConstant.ERROR)

        with mock.patch(NAMESPACE + '.path_check', return_value="a"), \
             mock.patch(NAMESPACE + '.generate_config', return_value=sample_config), \
             mock.patch(NAMESPACE + '.get_type_db_correspondences', return_value=None):
            res = pre_check_pmu_events_interface("", 0, "ai_core_profiling")
        self.assertEqual(res[0], NumberConstant.ERROR)

    def test_get_aicore_utilization_1(self):
        func_map = {"type_db_match": {"ai_core_profiling": "aicore_0.db"},
                    "sample_config": {"ai_core_profiling_mode": "task_based"}}
        with DBOpen("aicore_0.db") as db_open:
            with mock.patch(NAMESPACE + '.path_check', return_value="None"), \
                 mock.patch(NAMESPACE + '.pre_check_pmu_events_interface', return_value=(NumberConstant.ERROR, "", "")):
                res = get_aicore_utilization("", 0, NumberConstant.DEFAULT_NUMBER, NumberConstant.DEFAULT_START_TIME,
                                             NumberConstant.DEFAULT_END_TIME)
            self.assertEqual(len(json.loads(res)), 2)

            with mock.patch(NAMESPACE + '.path_check', return_value="None"), \
                 mock.patch(NAMESPACE + '.pre_check_pmu_events_interface', return_value=(0, "", func_map)), \
                 mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
                res = get_aicore_utilization("", 0, NumberConstant.DEFAULT_NUMBER, NumberConstant.DEFAULT_START_TIME,
                                             NumberConstant.DEFAULT_END_TIME)
            self.assertEqual(len(json.loads(res)), 2)

            with mock.patch(NAMESPACE + '.path_check', return_value="None"), \
                 mock.patch(NAMESPACE + '.pre_check_pmu_events_interface', return_value=(0, "", func_map)), \
                 mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)):
                res = get_aicore_utilization("", 0, NumberConstant.DEFAULT_NUMBER, NumberConstant.DEFAULT_START_TIME,
                                             NumberConstant.DEFAULT_END_TIME)
            self.assertEqual(len(json.loads(res)), 2)

    def test_get_aicore_utilization_2(self):
        func_map = {"type_db_match": {"ai_core_profiling": "aicore_0.db"},
                    "sample_config": {"ai_core_profiling_mode": "sample-based"}}

        create_sql = "CREATE TABLE IF NOT EXISTS AICoreOriginalData" \
                     "(mode, replayid, timestamp, coreid, task_cyc, event1, event2, event3, event4, event5, event6, event7, event8)"
        data = ((1, 0, 176256335449860, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),)
        with DBOpen("aicore_0.db") as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data("AICoreOriginalData", data)
            with mock.patch(NAMESPACE + '.path_check', return_value="test"), \
                 mock.patch(NAMESPACE + '.pre_check_pmu_events_interface', return_value=(0, "", func_map)), \
                 mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)):
                res = get_aicore_utilization("", 0, NumberConstant.DEFAULT_NUMBER, NumberConstant.DEFAULT_START_TIME,
                                             NumberConstant.DEFAULT_END_TIME)
            self.assertEqual(len(json.loads(res)), 2)


if __name__ == '__main__':
    unittest.main()
