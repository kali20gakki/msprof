import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant

from msmodel.hccl.hccl_model import HCCLModel
from profiling_bean.db_dto.hccl_dto import HcclDto
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
        data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
        col = ["hccl_name", "plane_id", "timestamp", "duration", "bandwidth",
               "stage", "step", "stream_id", "task_id", "task_type",
               "src_rank", "dst_rank", "notify_id", "transport_type", "size"]
        create_sql = "create table IF NOT EXISTS {0} " \
                     "(name TEXT, " \
                     "plane_id INTEGER, " \
                     "timestamp REAL, " \
                     "duration REAL, " \
                     "notify_id INTEGER, " \
                     "stage INTEGER, " \
                     "step INTEGER," \
                     "bandwidth REAL, " \
                     "stream_id INTEGER," \
                     "task_id INTEGER," \
                     "task_type TEXT," \
                     "src_rank INTEGER," \
                     "dst_rank INTEGER," \
                     "transport_type TEXT," \
                     "size TEXT)".format(DBNameConstant.TABLE_HCCL_ALL_REDUCE)
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
