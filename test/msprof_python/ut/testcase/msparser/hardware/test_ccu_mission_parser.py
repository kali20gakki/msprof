import os
import shutil
import struct
import unittest

from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from msparser.hardware.ccu_mission_parser import CCUMissionParser
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.ccu_mission_parser'


class TestCcuMissionParser(unittest.TestCase):
    file_list = {DataTag.CCU_MISSION: ['ccu0.instr.0.slice_0']}
    DIR_PATH = os.path.join(os.path.dirname(__file__), "ccu_mission")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    @classmethod
    def setUpClass(cls):
        InfoConfReader()._info_json = {"DeviceInfo": [{"hwts_frequency": "25.000000"}]}
        if not os.path.exists(cls.DATA_PATH):
            os.makedirs(cls.DATA_PATH)
            os.makedirs(cls.SQLITE_PATH)
            cls.make_ccu_mission_data()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_ccu_mission_data(cls):
        data = struct.pack("=16HQHQQ3H", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           139937251735, 693, 139937251707, 139937231292, 691, 1, 4)
        with FdOpen(os.path.join(cls.DATA_PATH, "ccu0.instr.0.slice_0"), operate="wb") as f:
            f.write(data)

    def test_read_binary_data_should_return_success(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        result = check.read_binary_data(os.path.join(self.DATA_PATH, "ccu0.instr.0.slice_0"))
        self.assertEqual(result, NumberConstant.SUCCESS)

    def test_parse(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        check.parse()

    def test_save(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        check.mission_data = [
            [1, 0, 0, 0, 0, 74, 133158368126, 15, 133158368126],
            [1, 0, 0, 0, 0, 74, 133158368126, 14, 133158368126]
        ]
        check.save()

    def test_ms_run(self):
        CCUMissionParser(self.file_list, self.CONFIG).ms_run()
