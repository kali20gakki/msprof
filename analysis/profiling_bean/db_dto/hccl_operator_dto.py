from common_func.constant import Constant


class HCCLOperatorDto:
    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._model_id = Constant.DEFAULT_INVALID_VALUE
        self._index_id = Constant.DEFAULT_INVALID_VALUE
        self._op_name = Constant.NA
        self._op_type = Constant.NA
        self._start_time = Constant.DEFAULT_INVALID_VALUE
        self._end_time = Constant.DEFAULT_INVALID_VALUE
        self._overlap_time = Constant.DEFAULT_INVALID_VALUE

    @property
    def model_id(self: any) -> any:
        """
        for model id
        """
        return self._model_id

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @property
    def index_id(self: any) -> any:
        """
        for index id
        """
        return self._index_id

    @index_id.setter
    def index_id(self: any, value: any) -> None:
        self._index_id = value

    @property
    def op_name(self: any) -> any:
        """
        for op name
        """
        return self._op_name

    @op_name.setter
    def op_name(self: any, value: any) -> None:
        self._op_name = value

    @property
    def op_type(self: any) -> any:
        """
        for op type
        """
        return self._op_type

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

    @property
    def start_time(self: any) -> any:
        """
        for start time
        """
        return self._start_time

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @property
    def end_time(self: any) -> any:
        """
        for duration
        """
        return self._end_time

    @end_time.setter
    def end_time(self: any, value: any) -> None:
        self._end_time = value

    @property
    def overlap_time(self: any) -> any:
        """
        for overlap time
        """
        return self._overlap_time

    @overlap_time.setter
    def overlap_time(self: any, value: any) -> None:
        self._overlap_time = value