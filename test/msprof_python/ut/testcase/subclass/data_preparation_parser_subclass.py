from msparser.aicpu.data_preparation_parser import DataPreparationParser


class DataPreparationParserSubclass(DataPreparationParser):
    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(file_list, sample_config)

    @staticmethod
    def read_data_queue(file_path: str) -> list:
        return DataPreparationParser._read_data_queue(file_path)

    def check_host_queue_mode(self: any, file_list: list) -> None:
        super()._check_host_queue_mode(file_list)

    def parse_host_queue(self: any, file_list: list) -> list:
        return super()._parse_host_queue(file_list)

    def parse_data_queue(self: any, file_list: list) -> list:
        return super()._parse_data_queue(file_list)

    def parse_data_queue_file(self: any, file_path: str) -> list:
        return super()._parse_data_queue_file(file_path)

    def get_data(self: any) -> dict:
        return self._data

    def get_host_queue_mode(self: any) -> int:
        return self._host_queue_mode
