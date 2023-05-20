import unittest
from unittest import mock

from msmodel.add_info.ctx_id_model import CtxIdModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.add_info.ctx_id_model'


class TestCtxIdModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.CtxIdModel.insert_data_to_db'):
            self.assertEqual(CtxIdModel('test').flush([]), None)


if __name__ == '__main__':
    unittest.main()
