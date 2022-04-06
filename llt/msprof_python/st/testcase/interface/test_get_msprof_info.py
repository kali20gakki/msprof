import unittest
from argparse import Namespace
from unittest import mock

from interface.get_msprof_info import MsprofInfoConstruct

NAMESPACE = 'interface.get_msprof_info'


class TestMsProfBasicInfo(unittest.TestCase):

    def test_main(self):
        args = ["get_msprof_info.py", "-dir", "test"]
        collection_path = {"collection_path": "test"}
        args_co = Namespace(**collection_path)
        with mock.patch(NAMESPACE + '.MsprofInfoConstruct.construct_argument_parser'), \
             mock.patch('sys.argv', args), \
             mock.patch('argparse.ArgumentParser.parse_args', return_value=args_co), \
             mock.patch(NAMESPACE + '.MsprofInfoConstruct.load_basic_info_model'):
            check = MsprofInfoConstruct()
            result = check.main()
        self.assertEqual(result, None)

        args = ["get_msprof_info.py"]
        with mock.patch(NAMESPACE + '.MsprofInfoConstruct.construct_argument_parser'), \
             mock.patch('sys.argv', args), \
             mock.patch('argparse.ArgumentParser.print_help'):
            check = MsprofInfoConstruct()
            result = check.main()
            self.assertEqual(result, None)


def test_main_1():
    args = ["msprof.py", "import" "-dir", "test"]
    collection_path = {"collection_path": "test"}
    args_co = Namespace(**collection_path)
    with mock.patch('sys.argv', args), \
         mock.patch('argparse.ArgumentParser.parse_args', return_value=args_co), \
         mock.patch('os.listdir', return_value=['start_info.0']), \
         mock.patch('profiling_bean.basic_info.msprof_basic_info.MsProfBasicInfo.init'), \
         mock.patch('profiling_bean.basic_info.msprof_basic_info.MsProfBasicInfo.run', return_value="{}"):
        MsprofInfoConstruct().main()

    args = ['msprof.py']
    with mock.patch('sys.argv', args):
        MsprofInfoConstruct().main()
