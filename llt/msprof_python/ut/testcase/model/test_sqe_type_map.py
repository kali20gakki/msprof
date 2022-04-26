import unittest
from model.sqe_type_map import SqeType

NAMESPACE = 'model.sqe_type_map'


class TestSqeType(unittest.TestCase):

    def test_init(self):
        SqeType(1)
