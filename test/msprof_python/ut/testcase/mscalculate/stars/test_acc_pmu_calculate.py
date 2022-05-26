import sqlite3
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from mscalculate.stars.acc_pmu_calculate import AccPmuCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from sqlite.db_manager import DBOpen

NAMESPACE = 'mscalculate.stars.acc_pmu_calculate'


class TestAccPmuCalculator(unittest.TestCase):
    sample_config = {"result_dir": ""}

    def test_ms_run(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        check = AccPmuCalculator({}, self.sample_config)
        check.ms_run()
        with mock.patch(NAMESPACE + '.AccPmuModel.check_table', return_value=True), \
             mock.patch(NAMESPACE + '.AccPmuCalculator.calculate'), \
             mock.patch(NAMESPACE + '.AccPmuCalculator.save'):
            check = AccPmuCalculator(file_list, self.sample_config)
            check.ms_run()
        with mock.patch(NAMESPACE + '.AccPmuModel.check_table', return_value=False), \
             mock.patch(NAMESPACE + '.AccPmuCalculator.calculate'), \
             mock.patch(NAMESPACE + '.AccPmuCalculator.save'):
            check = AccPmuCalculator(file_list, self.sample_config)
            check.ms_run()

    def test_calculate(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with DBOpen(DBNameConstant.DB_SOC_LOG) as db_open:
            with mock.patch(NAMESPACE + '.AccPmuCalculator._get_task_time_form_acsq', return_value=[]), \
                 mock.patch(NAMESPACE + '.ClassRowType.class_row', return_value=True), \
                 mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
                check = AccPmuCalculator(file_list, self.sample_config)
                check.model.cur = db_open.db_curs
                check.calculate()

    def test_save(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.AccPmuModel.drop_table'), \
             mock.patch(NAMESPACE + '.AccPmuModel.create_table'), \
             mock.patch(NAMESPACE + '.AccPmuModel.insert_data_to_db'):
            check = AccPmuCalculator(file_list, self.sample_config)
            check.save()

    def test_get_task_time_form_acsq(self):
        file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0', 'stars_soc.data.0.slice_1']}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(False, False)), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = AccPmuCalculator(file_list, self.sample_config)
            check.get_task_time_form_acsq()
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[[1, 2, 3]]), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = AccPmuCalculator(file_list, self.sample_config)
            res = check.get_task_time_form_acsq()
            self.assertEqual(res, {1: (2, 3)})
