import unittest
from msmodel.sqe_type_map import SqeType

NAMESPACE = 'msmodel.sqe_type_map'


class TestSqeType(unittest.TestCase):

    def test_init(self):
        SqeType(1)
