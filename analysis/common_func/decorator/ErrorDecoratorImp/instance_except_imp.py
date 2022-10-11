import traceback

from common_func.decorator.ErrorDecoratorImp.error_decorator_imp import ErrorDecoratorImp


class InstanceExceptImp(ErrorDecoratorImp):
    def __init__(self: any) -> None:
        super().__init__()

    def wrapper(self: any, *args, **kwargs) -> any:
        try:
            self.func(self.instance, *args, **kwargs)
        except self.exception_type as err:
            print(traceback.format_exc())
            return self.return_value