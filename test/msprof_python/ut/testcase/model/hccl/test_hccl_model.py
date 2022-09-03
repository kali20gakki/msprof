import unittest
from unittest import mock

from msmodel.hccl.hccl_model import HCCLModel
from profiling_bean.prof_enum.data_tag import DataTag

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

