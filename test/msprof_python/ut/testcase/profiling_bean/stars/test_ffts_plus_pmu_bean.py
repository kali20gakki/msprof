import unittest

from profiling_bean.stars.ffts_plus_pmu import FftsPlusPmuBean


class TestFftsPlusPmuBean(unittest.TestCase):
    def test_instance_of_ffts_plus_pmu_bean(self):
        args = (41, 1, 2, 3, 21474836484, 6, 7, 2312, 513, 17179869187, 5, 6, 8, 9, 4, 5, 8, 9, 7, 5, 7, 8, 10, 11)
        ffts_plus_pmu_bean = FftsPlusPmuBean(args)
        self.assertEqual([2, 3, 7, 0, 6, 8, (4, 5, 8, 9, 7, 5, 7, 8), (10, 11)], [ffts_plus_pmu_bean.stream_id,
                                                                                ffts_plus_pmu_bean.task_id,
                                                                                ffts_plus_pmu_bean.subtask_id,
                                                                                ffts_plus_pmu_bean.ffts_type,
                                                                                ffts_plus_pmu_bean.subtask_type,
                                                                                ffts_plus_pmu_bean.total_cycle,
                                                                                ffts_plus_pmu_bean.pmu_list,
                                                                                ffts_plus_pmu_bean.time_list])
        self.assertTrue(ffts_plus_pmu_bean.is_aic_data())

    def test_instance_of_ffts_plus_pmu_bean_is_no_aic_data(self):
        args = (41, 1, 2, 3, 21474836484, 6, 7, 23111, 513, 17179869187, 5, 6, 8, 9, 4, 5, 8, 9, 7, 5, 7, 8)
        ffts_plus_pmu_bean = FftsPlusPmuBean(args)
        self.assertEqual([2, 3, 7, 2, 6, 8, (4, 5, 8, 9, 7, 5, 7, 8), ()], [ffts_plus_pmu_bean.stream_id,
                                                                                ffts_plus_pmu_bean.task_id,
                                                                                ffts_plus_pmu_bean.subtask_id,
                                                                                ffts_plus_pmu_bean.ffts_type,
                                                                                ffts_plus_pmu_bean.subtask_type,
                                                                                ffts_plus_pmu_bean.total_cycle,
                                                                                ffts_plus_pmu_bean.pmu_list,
                                                                                ffts_plus_pmu_bean.time_list])
        self.assertFalse(ffts_plus_pmu_bean.is_aic_data())
