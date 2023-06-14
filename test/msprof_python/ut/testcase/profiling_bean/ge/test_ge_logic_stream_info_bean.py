import unittest
import struct
from unittest import mock

from msparser.ge.ge_logic_stream_info_bean import GeLogicStreamInfoBean

NAMESPACE = 'msmodel.ge.ge_logic_stream_model'


class TestGeLogicStreamInfoBean(unittest.TestCase):
    def test_construct(self):
        args = [23130, 0, 0, 0, 0, 0, 1, 2, *(120,) * 48]
        bean = GeLogicStreamInfoBean(args)
        self.assertEqual(bean.logic_stream_id, 1)
        self.assertEqual(bean.physic_stream_id, '120,120')


if __name__ == '__main__':
    unittest.main()
