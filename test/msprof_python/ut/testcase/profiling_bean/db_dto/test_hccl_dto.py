import unittest

from profiling_bean.db_dto.acc_pmu_dto import AccPmuOriDto
from profiling_bean.db_dto.hccl_dto import HcclDto

NAMESPACE = 'profiling_bean.db_dto.acc_pmu_dto'


class TestHcclDto(unittest.TestCase):

    def test_construct(self):
        data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
        col = ["_hccl_name", "_plane_id", "_timestamp", "_duration", "_bandwidth",
               "_stage", "_step", "_stream_id", "_task_id", "_task_type",
               "_src_rank", "_dst_rank", "_notify_id", "_transport_type", "_size"]
        test = HcclDto()
        for index, i in enumerate(data):
            if hasattr(test, col[index]):
                setattr(test, col[index], i)
        self.assertEqual(test.hccl_name, 1)
        self.assertEqual(test.plane_id, 2)
        self.assertEqual(test.timestamp, 3)
        self.assertEqual(test.duration, 4)
        self.assertEqual(test.bandwidth, 5)
        self.assertEqual(test.stage, 6)
        self.assertEqual(test.step, 7)
        self.assertEqual(test.stream_id, 8)
        self.assertEqual(test.task_id, 9)
        self.assertEqual(test.task_type, 10)
        self.assertEqual(test.src_rank, 11)
        self.assertEqual(test.dst_rank, 12)
        self.assertEqual(test.notify_id, 13)
        self.assertEqual(test.transport_type, 14)
        self.assertEqual(test.size, 15)


if __name__ == '__main__':
    unittest.main()
