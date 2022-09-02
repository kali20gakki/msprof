import unittest

from profiling_bean.ge.ge_session_bean import GeSessionInfoBean

NAMESPACE = 'profiling_bean.struct_info.ge.ge_fusion_op_info_bean'


class TestGeModelTimeBean(unittest.TestCase):

    def test_construct(self):
        bean = GeSessionInfoBean()
        binary_data = b'ZZ\x1a\x00\x01\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00$\xcda\xf5\xa2Z\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        bean.fusion_decode(binary_data)
        self.assertEqual(bean.graph_id, 1)
        self.assertEqual(bean.mod, 0)
        self.assertEqual(bean.model_id, 1)
        self.assertEqual(bean.session_id, 0)
        self.assertEqual(bean.model_id, 1)
        self.assertEqual(bean.timestamp, 99655948029220)


if __name__ == '__main__':
    unittest.main()
