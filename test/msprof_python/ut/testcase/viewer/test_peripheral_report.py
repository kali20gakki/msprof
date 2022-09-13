import sqlite3
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBOpen
from viewer.peripheral_report import get_peripheral_dvpp_data
from viewer.peripheral_report import get_peripheral_nic_data

NAMESPACE = 'viewer.peripheral_report'
configs = {"headers": "Dvpp Id,Engine Type,Engine ID,All Time(us),All Frame,All Utilization(%)",
           "columns": "duration,bandwidth,rxBandwidth,rxPacket,rxErrorRate,rxDroppedRate,txBandwidth,txPacket,txErrorRate,txDroppedRate,funcId"}


class TestHardwareInfoView(unittest.TestCase):
    def test_get_peripheral_dvpp_data_1(self):
        with DBOpen(DBNameConstant.DB_PERIPHERAL) as _db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
                res = get_peripheral_dvpp_data('', "DvppReportData", 0, configs)
            self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with DBOpen(DBNameConstant.DB_PERIPHERAL) as _db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(_db_open.db_conn, _db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.GetTableData.get_data_count_for_device', side_effect=sqlite3.Error):
                res = get_peripheral_dvpp_data('', "DvppReportData", 0, configs)
            self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_peripheral_dvpp_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS DvppReportData" \
                     " (dvppid, device_id, enginetype, engineid, alltime, allframe, allutilization)"
        data = ((8, 0, "0", 3, 0.0, 0.0, "0.0%"),)
        with DBOpen(DBNameConstant.DB_PERIPHERAL) as _db_open:
            _db_open.create_table(create_sql)
            _db_open.insert_data("DvppReportData", data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(_db_open.db_conn, _db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
                res = get_peripheral_dvpp_data('', "DvppReportData", 0, configs)
            self.assertEqual(res[2], 1)

    def test_get_peripheral_nic_data_1(self):
        with DBOpen("nic.db") as _db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
                res = get_peripheral_nic_data('', "NicReportData", 0, configs)
            self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)
        with DBOpen("nic.db") as _db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(_db_open.db_conn, _db_open.db_curs)), \
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
        with DBOpen("nic.db") as _db_open:
            _db_open.create_table(create_sql)
            _db_open.insert_data("NicReportData", data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(_db_open.db_conn, _db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
                res = get_peripheral_nic_data('', "NicReportData", 0, configs)
            self.assertEqual(res[2], 1)


if __name__ == '__main__':
    unittest.main()
