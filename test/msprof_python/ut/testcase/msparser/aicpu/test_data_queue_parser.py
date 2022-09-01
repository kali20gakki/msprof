import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.aicpu.data_queue_parser import DataQueueParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.aicpu.data_queue_parser'


class TestDataQueueParser(unittest.TestCase):
    file_list = {DataTag.DATA_QUEUE: ['DATA_PREPROCESS.AICPUMI.3.slice_0']}

    def test_parse(self):
        data = b'Node:GetNext_dequeue_wait, queue size:108, Run start:65347643713, ' \
               b'Run end:65347643732\n\x00Node:GetNext_dequeue_wait, queue size:107, ' \
               b'Run start:65347648468, Run end:65347648517\n\x00Node:GetNext_dequeue_wait, ' \
               b'queue size:106, Run start:65347649639, Run end:65347649655'
        with mock.patch(NAMESPACE + '.check_file_readable'), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch("builtins.open", mock.mock_open(read_data=data)):
            DataQueueParser(self.file_list, CONFIG).ms_run()
        with mock.patch(NAMESPACE + '.check_file_readable'), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', side_effect=OSError):
            DataQueueParser(self.file_list, CONFIG).ms_run()
