import unittest

from profiling_bean.struct_info.struct_decoder import StructDecoder

NAMESPACE = 'profiling_bean.struct_info.struct_decoder'
args = [0, 0, 0, 0, 0, 0, 0]


class TestStepTrace(unittest.TestCase):
    def test_init(self):
        StructDecoder()


if __name__ == '__main__':
    unittest.main()
