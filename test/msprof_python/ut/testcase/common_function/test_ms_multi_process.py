import unittest
from unittest import mock

from common_func.msprof_exception import ProfException
from common_func.ms_multi_process import MsMultiProcess


NAMESPACE = 'common_func.ms_multi_process'
CONFIG = {'result_dir': 'output'}


class TestMsMultiProcess(unittest.TestCase):

    def test_run_raise_profexception(self) -> None:
        for exception in [ProfException(13, "data exceed limit"), ProfException(2), RuntimeError()]:
            with mock.patch(NAMESPACE + '.MsMultiProcess.process_init'), \
                    mock.patch(NAMESPACE + '.MsMultiProcess.ms_run', side_effect=exception):
                test = MsMultiProcess(CONFIG)
                test.run()


if __name__ == '__main__':
    unittest.main()
