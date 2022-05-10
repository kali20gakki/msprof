import unittest

from profiling_bean.ge.ge_step_bean import GeStepBean

NAMESPACE = 'profiling_bean.struct_info.ge.ge_fusion_op_info_bean'


class TestGeModelTimeBean(unittest.TestCase):

    def test_construct(self):
        bean = GeStepBean()
        binary_data = b'ZZ\x19\x00\x01\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\\\xf1n\xf5\xa2Z\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\xe9\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        bean.fusion_decode(binary_data)
        self.assertEqual(bean.index_num, 1)
        self.assertEqual(bean.model_id, 1)
        self.assertEqual(bean.stream_id, 5)
        self.assertEqual(bean.tag, 0)
        self.assertEqual(bean.model_id, 1)
        self.assertEqual(bean.task_id,1)
        self.assertEqual(bean.thread_id, 489)
        self.assertEqual(bean.timestamp, 99655948890460)
        self.assertEqual(bean.thread_id, 489)


if __name__ == '__main__':
    unittest.main()
