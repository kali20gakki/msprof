import abc


class ErrorDecoratorImp:
    def __init__(self: any) -> None:
        self.exception_type = None
        self.return_value = None
        self.func = None
        self.instance = None
        self.owner = None

    def __get__(self: any, instance: any, owner: any) -> any:
        self.instance = instance
        self.owner = owner
        return self.wrapper

    @abc.abstractmethod
    def wrapper(self: any, *args, **kwargs) -> any:
        """
        wrapper
        :param args:
        :param kwargs:
        :return:
        """