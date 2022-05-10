import unittest
from unittest import mock

from saver.training.time_saver import TimeSaver
from sqlite.db_manager import DBManager
import sqlite3

NAMESPACE = 'saver.training.time_saver'


class TestTimeSaver(unittest.TestCase):

    def test_save_data_to_db(self):
        message = {"data": ({"dev_mon": 1, "dev_wall": 2, "dev_cntvct": 3, "host_mon": 4,
                             "host_wall": 5, "device_id": 1, "dev_mon": 2, "dev_wall": 3,
                             "dev_cntvct": 4, "host_mon": 5, "host_wall": 6},),
                   "sql_path": ((1, 2, 3, 4, 5, 6),)}
        create_sql = "create table IF NOT EXISTS time (device_id int, dev_mon int, dev_wall int, " \
                     "dev_cntvct int,host_mon int, host_wall int)"
        data = (
            (0, 100941307149, 1616557556918975294, 4764985523, 117891403077, 1616557556918975294),)
        insert_sql = "insert into {0} values ({value})".format("time",
                                                               value="?," * (
                                                                       len(data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("time.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.check_number_valid', return_value=True), \
                mock.patch(NAMESPACE + '.TimeSaver.create_timedb_conn', return_value=res):
            key = TimeSaver()
            key.save_data_to_db(message)

    def test_create_timedb_conn(self):
        sql_path = "time.db"
        with mock.patch('os.path.join', return_value="time.db"), \
                mock.patch(NAMESPACE + '.sqlite3.connect', return_value=True):
            key = TimeSaver()
            result = key.create_timedb_conn(sql_path)
        self.assertEqual(len(result), 2)


if __name__ == '__main__':
    unittest.main()
