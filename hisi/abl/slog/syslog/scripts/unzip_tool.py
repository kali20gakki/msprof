#!/usr/bin/env python
# coding=utf-8
"""
Function: unzip file tool by gzip
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""

import getopt
import gzip
import os
import stat
import sys


class GZipTool:
    """
    tool dor Gzip
    """
    ZIP_FILE_POSTFIX = '.log'
    UNZIP_FILE_POSTFIX = '.out'
    WRITE_MODES = stat.S_IWUSR | stat.S_IRUSR | stat.S_IRGRP
    WRITE_FLAGS = os.O_WRONLY | os.O_CREAT

    def __init__(self: any, buf_size: int) -> None:
        self.buf_size = buf_size
        self.fin = None
        self.fout = None

    @staticmethod
    def __check_file_permission(src_file_path: str) -> bool:
        if not os.access(src_file_path, os.F_OK):
            print("Zipfile(%s) is not exist!" % src_file_path)
            return False
        if not os.access(src_file_path, os.R_OK):
            print("Zipfile(%s) is not accessible to read!" % src_file_path)
            return False
        if os.path.getsize(src_file_path) == 0:
            print("Zipfile(%s) file size = 0!" % src_file_path)
            return False
        return True

    def decompress_file(self: any, src_file_path: str) -> None:
        """
        decompress files
        :param src_file_path: src file path
        :return: None
        """
        dst_file_path = src_file_path + self.UNZIP_FILE_POSTFIX
        if not self.__check_file_permission(src_file_path):
            return

        if not self.__load_fin(src_file_path):
            return

        if not self.__load_fout(dst_file_path):
            return

        if self.__in2out(src_file_path, dst_file_path) == 0:
            print("Unzip zipfile(%s) to unzipfile(%s) succeed!" % (src_file_path, dst_file_path))
            os.chmod(dst_file_path, stat.S_IRUSR | stat.S_IRGRP)

    def __load_fout(self: any, dst_file_path: str) -> bool:
        try:
            self.fout = os.fdopen(os.open(dst_file_path, self.WRITE_FLAGS, self.WRITE_MODES), 'wb')
            return True
        except IOError:
            print("Open unzipfile(%s) fail by write mode!" % dst_file_path)
            self.fin.close()
            return False
        finally:
            pass

    def __load_fin(self: any, src_file_path: str) -> bool:
        try:
            self.fin = gzip.open(src_file_path, 'rb')
            return True
        except IOError:
            print("Open zipfile(%s) fail by read mode!" % src_file_path)
            return False
        finally:
            pass

    def __in2out(self: any, src_file_path: str, dst_file_path: str) -> int:
        while True:
            try:
                buf = self.fin.read(self.buf_size)
            except IOError:
                print("Read zipfile(%s) fail, maybe file isnot gzip format!" % src_file_path)
                self.fin.close()
                self.fout.close()
                if os.path.getsize(dst_file_path) == 0:
                    os.remove(dst_file_path)
                return -1
            finally:
                pass
            if len(buf) < 1:
                break
            if not self.__write_data(buf, dst_file_path):
                return -1
        return 0 if self.__close_resource(dst_file_path, src_file_path) else -1

    def __write_data(self: any, buf: int, dst_file_path: str) -> bool:
        try:
            self.fout.write(buf)
            return True
        except IOError:
            print("Write data to unzipfile(%s) fail!" % dst_file_path)
            self.fin.close()
            self.fout.close()
            if os.path.getsize(dst_file_path) == 0:
                os.remove(dst_file_path)
            return False
        finally:
            pass

    def __close_resource(self: any, dst_file_path: str, src_file_path: str) -> bool:
        try:
            self.fin.close()
            self.fout.close()
            return True
        except IOError:
            print("Close zipfile(%s) or unzipfile(%s) fail!" % (src_file_path, dst_file_path))
            return False
        finally:
            pass

    def decompress_directory(self: any, src_file_path: str) -> None:
        """
        decompress directory
        :param src_file_path: src file path
        :return: None
        """
        if not os.path.isdir(src_file_path):
            print("Zipfile directory(%s) is not exist and exit!" % src_file_path)
            return
        for filename in os.listdir(src_file_path):
            if not os.path.isdir(os.path.join(src_file_path, filename)):
                if filename.endswith(self.ZIP_FILE_POSTFIX):
                    self.decompress_file(os.path.join(src_file_path, filename))


def print_helpinfo() -> None:
    """
    print help info
    :return: None
    """
    print('*********************help info*********************')
    print('python unzip_tool.py -f <logfile>')
    print('python unzip_tool.py -d <logdirectory>')
    print('eg: python unzip_tool.py -f /var/log/npu/slog/host-0/zip.log')
    print('eg: python unzip_tool.py -d /var/log/npu/slog/host-0')
    print('*********************help info*********************')


def main(argv: any) -> None:
    """
    entrance for unzip
    :param argv: params
    :return:None
    """
    input_file = ''
    is_file = 0
    buf_size = 1024 * 8
    opts = get_options(argv)
    for opt, arg in opts:
        if opt == '-h':
            print_helpinfo()
            sys.exit()
        elif opt in ("-f", "--file"):
            input_file = arg
            is_file = 1
        elif opt in ("-d", "--directory"):
            input_file = arg
    gziptool = GZipTool(buf_size)
    if input_file == '':
        print_helpinfo()
        sys.exit()
    if is_file == 1:
        print('*********************unzip file start*********************')
        gziptool.decompress_file(input_file)
        print('*********************unzip file end***********************')
    else:
        print('*********************unzip file start*********************')
        gziptool.decompress_directory(input_file)
        print('*********************unzip file end***********************')


def get_options(argv: any) -> any:
    """
    get options from param
    :param argv: param
    :return:
    """
    try:
        opts, _ = getopt.getopt(argv, "hf:d:")
        return opts
    except getopt.GetoptError:
        print_helpinfo()
        sys.exit()
    finally:
        pass


if __name__ == '__main__':
    main(sys.argv[1:])
