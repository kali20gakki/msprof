import sqlite3
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBManager
from viewer.peripheral_report import get_peripheral_dvpp_data, get_peripheral_nic_data

NAMESPACE = 'viewer.peripheral_report'
configs = {"headers": "Dvpp Id,Engine Type,Engine ID,All Time(us),All Frame,All Utilization(%)",
           "columns": "duration,bandwidth,rxBandwidth,rxPacket,rxErrorRate,rxDroppedRate,txBandwidth,txPacket,txErrorRate,txDroppedRate,funcId"}


class TestHardwareInfoView(unittest.TestCase):
    def test_get_peripheral_dvpp_data_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_PERIPHERAL)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_peripheral_dvpp_data('', "DvppReportData", 0, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.GetTableData.get_data_count_for_device', side_effect=sqlite3.Error):
            res = get_peripheral_dvpp_data('', "DvppReportData", 0, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_peripheral_dvpp_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS DvppReportData" \
                     " (dvppid, device_id, enginetype, engineid, alltime, allframe, allutilization)"
        data = ((8, 0, "0", 3, 0.0, 0.0, "0.0%"),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("DvppReportData", data)
        test_sql = db_manager.create_table(DBNameConstant.DB_PERIPHERAL, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_peripheral_dvpp_data('', "DvppReportData", 0, configs)
        test_sql = db_manager.connect_db(DBNameConstant.DB_PERIPHERAL)
        (test_sql[1]).execute("drop Table DvppReportData")
        db_manager.destroy(test_sql)
        self.assertEqual(res[2], 1)

    def test_get_peripheral_nic_data_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table("nic.db")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_peripheral_nic_data('', "NicReportData", 0, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.GetTableData.get_table_data_for_device', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.GetTableData.get_data_count_for_device', side_effect=sqlite3.Error):
            res = get_peripheral_nic_data('', "NicReportData", 0, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_peripheral_nic_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS NicReportData" \
                     " (rowid, device_id, duration, bandwidth, rxbandwidth, txbandwidth, rxpacket, rxerrorrate, " \
                     "rxdroppedrate, txpacket, txerrorrate, txdroppedrate, funcid)"
        data = ((1, 0, 12303986860.0, 100000, 0.0, 0.0, 0.0, "0%", "0%", 0.0, "0%", "0%", 0),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("NicReportData", data)
        test_sql = db_manager.create_table("nic.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_peripheral_nic_data('', "NicReportData", 0, configs)
        test_sql = db_manager.connect_db("nic.db")
        (test_sql[1]).execute("drop Table NicReportData")
        db_manager.destroy(test_sql)
        self.assertEqual(res[2], 1)


if __name__ == '__main__':
    unittest.main()
