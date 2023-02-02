import unittest

from profiling_bean.db_dto.msproftx_dto import MsprofTxDto


class TestMsprofTxDto(unittest.TestCase):
    def test_init(self: any) -> None:
        msproftx_dto = MsprofTxDto()

        msproftx_dto.pid = 0
        msproftx_dto.tid = 0
        msproftx_dto.category = 0
        msproftx_dto.event_type = 0
        msproftx_dto.payload_type = 0
        msproftx_dto.start_time = 0
        msproftx_dto.end_time = 0
        msproftx_dto.message_type = 0
        msproftx_dto.message = 0
        msproftx_dto.call_trace = 0
        msproftx_dto.dur_time = 0

        self.assertEqual((msproftx_dto.pid,
              msproftx_dto.tid,
              msproftx_dto.category,
              msproftx_dto.event_type,
              msproftx_dto.payload_type,
              msproftx_dto.start_time,
              msproftx_dto.end_time,
              msproftx_dto.message_type,
              msproftx_dto.message,
              msproftx_dto.call_trace,
              msproftx_dto.dur_time),
                         (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
