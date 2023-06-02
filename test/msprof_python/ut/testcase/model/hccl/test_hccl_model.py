import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.platform.chip_manager import ChipManager

from msmodel.hccl.hccl_model import HCCLModel
from msmodel.hccl.hccl_model import HcclViewModel
from profiling_bean.db_dto.hccl_dto import HcclDto
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag
from sqlite.db_manager import DBOpen

NAMESPACE = 'msmodel.hccl.hccl_model'


class TestHCCLModel(unittest.TestCase):
    sample_config = {'result_dir': '/tmp/result',
                     'tag_id': 'JOBEJGBAHABDEEIJEDFHHFAAAAAAAAAA',
                     'device_id': '127.0.0.1'
                     }
    file_list = {DataTag.HCCL: ['HCCL.hcom_allReduce_1_1_1.1.slice_0']}

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HCCLModel.insert_data_to_db'):
            HCCLModel("", [" "]).flush([])

    def test_get_hccl_data(self):
        data = [1, 2, 3, 4,
                "{'notify id': 4294967840, 'duration estimated': 0.8800048828125, 'stage': 4294967295, "
                "'step': 4294967385, 'bandwidth': 'NULL', 'stream id': 8, 'task id': 34, 'task type': 'Notify Record',"
                " 'src rank': 2, 'dst rank': 1, 'transport type': 'SDMA', 'size': None, 'tag': 'all2allvc_1_5'}"]
        col = ["hccl_name", "plane_id", "timestamp", "duration", "args"]
        create_sql = "create table IF NOT EXISTS {0} " \
                     "(name TEXT, " \
                     "plane_id INTEGER, " \
                     "timestamp REAL, " \
                     "duration REAL, " \
                     "args TEXT)".format(DBNameConstant.TABLE_HCCL_ALL_REDUCE)
        test = HcclDto()
        for index, i in enumerate(data):
            if hasattr(test, col[index]):
                setattr(test, col[index], i)
        with DBOpen(DBNameConstant.DB_HCCL) as db_open:
            db_open.create_table(create_sql)
            with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[test]):
                check = HCCLModel("", [DBNameConstant.TABLE_HCCL_ALL_REDUCE])
                check.cur = db_open.db_curs
                check.get_hccl_data()

    def test_get_task_db_and_table_when_given_different_chip_model_then_return_task_time_sql_info(self):
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        ret = HcclViewModel.get_task_db_and_table()
        self.assertEqual(DBNameConstant.DB_RUNTIME, ret.db_name)

        ChipManager().chip_id = ChipModel.CHIP_V2_1_0
        ret = HcclViewModel.get_task_db_and_table()
        self.assertEqual(DBNameConstant.DB_HWTS, ret.db_name)

        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        ret = HcclViewModel.get_task_db_and_table()
        self.assertEqual(DBNameConstant.DB_HWTS, ret.db_name)

        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = HcclViewModel.get_task_db_and_table()
        self.assertEqual(DBNameConstant.DB_SOC_LOG, ret.db_name)

        # invalid chip model type
        ChipManager().chip_id = 1000
        ret = HcclViewModel.get_task_db_and_table()
        self.assertEqual('', ret.db_name)

    def test_get_hccl_communication_data_when_given_attach_to_db_failed_then_return_empty_list(self):
        ChipManager().chip_id = 1000
        check = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_ALL_REDUCE])
        ret = check.get_hccl_communication_data()
        self.assertEqual([], ret)

        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=False):
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            check = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_ALL_REDUCE])
            ret = check.get_hccl_communication_data()
            self.assertEqual([], ret)

    def test_get_hccl_communication_data_when_given_network_scene_then_return_data_list(self):
        with mock.patch(NAMESPACE + '.HcclViewModel.attach_to_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data'), \
                mock.patch('common_func.utils.Utils.get_scene', return_value=Constant.STEP_INFO):
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            check = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_ALL_REDUCE])
            check.get_hccl_communication_data()

    def test_get_hccl_op_data_sql(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_ALL_REDUCE])
            check.get_hccl_op_data()

    def test_get_hccl_op_time_section_sql(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HcclViewModel("", DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_ALL_REDUCE])
            check.get_hccl_op_time_section()
