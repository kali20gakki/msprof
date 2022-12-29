#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
import os

from common_func.constant import Constant
from common_func.path_manager import PathManager
from profiling_bean.struct_info.aic_pmu import AicPmuBean


class OffsetCalculator:
    """
    class used to calculate the offset of aging_data
    """

    def __init__(self: any, file_list: list, struct_size: int, project_path: str) -> None:
        self.file_list = file_list
        self.project_path = project_path
        self.struct_size = struct_size
        self.last_cache = bytes()
        self.has_read = False

    def calculate_total_offset(self: any) -> int:
        """
        calculate file_list offset
        :return: total_size % struct_size
        """
        total_size = 0
        if self.file_list:
            self.has_read = True
        for file in self.file_list:
            total_size += os.path.getsize(PathManager.get_data_file_path(self.project_path, file))
        return total_size % self.struct_size

    def pre_process(self: any, file_reader: any, file_size: int) -> bytes:
        """
        Dealing with aging and truncated data
        :return: bytes
        """
        if file_size < 0 or file_size > Constant.MAX_READ_FILE_BYTES:
            logging.error("Invalid file size %s for offset pre process", file_size)
            return bytes()
        if not self.has_read:
            offset = self.calculate_total_offset()
            file_reader.read(offset)
            file_size -= offset
        complete_file = self.last_cache + file_reader.read(file_size)
        left_size = (len(complete_file) % self.struct_size)
        if left_size:
            self.last_cache = complete_file[-left_size:]
        else:
            self.last_cache = bytes()
        return complete_file[:(len(complete_file) - len(self.last_cache))]


class FileCalculator(OffsetCalculator):
    """
    class used to calculate file index and offset
    """

    def __init__(self: any, *args: any) -> None:
        file_list, struct_size, project_path, offset_count, total_count = args
        super().__init__(file_list, struct_size, project_path)
        self._total_count = total_count
        self._offset_count = offset_count
        self._current_count = 0
        self._file_cache = bytes()
        self._is_first_file = False

    def prepare_process(self: any) -> bytes:
        """
        prepare to process data
        :return: data bytes
        """
        _total_sum_size = self._get_sum_size()
        _total_need_size = self._get_total_need_size()
        for _file in self.file_list:
            _file = PathManager.get_data_file_path(self.project_path, _file)
            _file_size = os.path.getsize(_file)
            if _total_sum_size - _file_size >= 0:
                _total_sum_size -= _file_size
                continue

            # get left offset, and the file seek to offset
            _file_offset = self._get_file_offset(_total_sum_size)
            # total sum size need to be reset while the file that start to read had found.
            _total_sum_size = 0
            # then calculate least file
            if _total_need_size > _file_size - _file_offset:
                _total_need_size -= (_file_size - _file_offset)
                self._read_binary_file(_file, _file_offset, _file_size)
            else:
                self._read_binary_file(_file, _file_offset, _file_offset + _total_need_size)
                return self._file_cache
        return self._file_cache

    def _get_sum_size(self: any) -> int:
        """
        :return: file offset
        """
        return self.struct_size * self._offset_count + self.calculate_total_offset()

    def _get_total_need_size(self: any) -> int:
        return self._total_count * self.struct_size

    def _get_file_offset(self: any, total_sum_size: int) -> int:
        if not self._is_first_file:
            self._is_first_file = True
            return total_sum_size
        return 0

    def _read_binary_file(self: any, _file: str, left_offset: int, right_offset: int) -> any:
        with open(_file, 'rb') as _file_reader:
            _file_reader.seek(left_offset)
            self._file_cache += _file_reader.read(right_offset - left_offset)


class FileReverseCalculator:
    """
    find the last ai core task which match the stream and task id of last hwts ai core task
    return the discard aicore count from tail
    """
    STREAM_TASK_KEY_FMT = "{0}-{1}"

    def __init__(self: any, project_path, file_list, struct_size, repeat_times, aic_stream_task) -> None:
        self._project_path = project_path
        self._file_list = file_list
        self._struct_size = struct_size
        self._last_cache = bytes()
        self.target_aic_str = self.STREAM_TASK_KEY_FMT.format(aic_stream_task[0], aic_stream_task[1])
        self.tail_aic_cnt = 0
        self.is_find = False
        self.repeat_times = repeat_times

    def read_file_from_tail(self: any):
        for _file in self._file_list[::-1]:
            _file = PathManager.get_data_file_path(self._project_path, _file)
            with open(_file, 'rb') as _aic_reader:
                _file_size = os.path.getsize(_file)
                complete_file = _aic_reader.read(_file_size) + self._last_cache
                left_size = len(complete_file) % self._struct_size
                if left_size:
                    self._last_cache = complete_file[:left_size]
                else:
                    self._last_cache = bytes()
                read_file = complete_file[left_size:]
                self.read_chunk_from_tail(read_file)
            if self.is_find:
                break

    def read_chunk_from_tail(self, read_file: bytes):
        total_bytes_size = len(read_file)
        for byte_end_pos in range(total_bytes_size, 0, -self._struct_size):
            chunk = read_file[byte_end_pos - self._struct_size: byte_end_pos]
            _aic_log = AicPmuBean.decode(chunk)
            if self.target_aic_str == self.STREAM_TASK_KEY_FMT.format(_aic_log.stream_id, _aic_log.task_id):
                if self.repeat_times == 0:
                    self.is_find = True
                    return
                else:
                    self.repeat_times -= 1
            self.tail_aic_cnt += 1
