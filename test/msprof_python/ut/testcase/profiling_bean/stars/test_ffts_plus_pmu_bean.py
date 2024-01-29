import unittest

from profiling_bean.stars.ffts_block_pmu import FftsBlockPmuBean


class TestFftsPlusPmuBean(unittest.TestCase):
    def test_instance_of_ffts_plus_pmu_bean(self):
        args = (
            617, 27603, 3, 2, 0, 6, 0, 55, 128, 0, 0, 1, 1, 642379,
            0, 28, 0, 28384, 0, 920, 209, 2584, 121, 2201803260082, 2201803277549
        )
        ffts_plus_pmu_bean = FftsBlockPmuBean(args)
        self.assertEqual([3, 2, 6, 0, 1, 27, 4, 1, 1, 642379,
                          (28, 0, 28384, 0, 920, 209, 2584, 121), (2201803260082, 2201803277549)],
                         [ffts_plus_pmu_bean.stream_id,
                          ffts_plus_pmu_bean.task_id,
                          ffts_plus_pmu_bean.subtask_type,
                          ffts_plus_pmu_bean.subtask_id,
                          ffts_plus_pmu_bean.core_type,
                          ffts_plus_pmu_bean.core_id,
                          ffts_plus_pmu_bean.ffts_type,
                          ffts_plus_pmu_bean.sub_block_id,
                          ffts_plus_pmu_bean.block_id,
                          ffts_plus_pmu_bean.total_cycle,
                          ffts_plus_pmu_bean.pmu_list,
                          ffts_plus_pmu_bean.time_list])
        self.assertTrue(ffts_plus_pmu_bean.is_aic_data())

    def test_instance_of_ffts_plus_pmu_bean_is_no_aic_data_should_return_false(self):
        args = (
            617, 27603, 3, 4, 0, 6, 0, 54, 81, 0, 0, 1, 1, 642379,
            0, 28, 0, 28384, 0, 920, 209, 2584, 121, 2201803260083, 2201803277550
        )
        ffts_plus_pmu_bean = FftsBlockPmuBean(args)
        self.assertEqual([3, 4, 6, 0, 0, 27, 2, 1, 1, 642379,
                          (28, 0, 28384, 0, 920, 209, 2584, 121), (2201803260083, 2201803277550)],
                         [ffts_plus_pmu_bean.stream_id,
                          ffts_plus_pmu_bean.task_id,
                          ffts_plus_pmu_bean.subtask_type,
                          ffts_plus_pmu_bean.subtask_id,
                          ffts_plus_pmu_bean.core_type,
                          ffts_plus_pmu_bean.core_id,
                          ffts_plus_pmu_bean.ffts_type,
                          ffts_plus_pmu_bean.sub_block_id,
                          ffts_plus_pmu_bean.block_id,
                          ffts_plus_pmu_bean.total_cycle,
                          ffts_plus_pmu_bean.pmu_list,
                          ffts_plus_pmu_bean.time_list])
        self.assertFalse(ffts_plus_pmu_bean.is_aic_data())

    def test_instance_of_ffts_plus_pmu_bean_is_ffts_mix_aic_data_should_return_true(self):
        args = (
            617, 27603, 3, 4, 0, 6, 0, 54, 128, 0, 0, 1, 1, 642379,
            0, 28, 0, 28384, 0, 920, 209, 2584, 121, 2201803260084, 2201803277551
        )
        ffts_plus_pmu_bean = FftsBlockPmuBean(args)
        self.assertEqual([3, 4, 6, 0, 0, 27, 4, 1, 1, 642379,
                          (28, 0, 28384, 0, 920, 209, 2584, 121), (2201803260084, 2201803277551)],
                         [ffts_plus_pmu_bean.stream_id,
                          ffts_plus_pmu_bean.task_id,
                          ffts_plus_pmu_bean.subtask_type,
                          ffts_plus_pmu_bean.subtask_id,
                          ffts_plus_pmu_bean.core_type,
                          ffts_plus_pmu_bean.core_id,
                          ffts_plus_pmu_bean.ffts_type,
                          ffts_plus_pmu_bean.sub_block_id,
                          ffts_plus_pmu_bean.block_id,
                          ffts_plus_pmu_bean.total_cycle,
                          ffts_plus_pmu_bean.pmu_list,
                          ffts_plus_pmu_bean.time_list])
        self.assertTrue(ffts_plus_pmu_bean.is_ffts_mix_aic_data())
