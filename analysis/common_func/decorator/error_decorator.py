class ErrorDecorator:
    def __init__(self: any, exception_type: any, method_type: any, return_value: any = None) -> any:
        self.imp = method_type()
        self.imp.exception_type = exception_type
        self.imp.return_value = return_value

    def __call__(self: any, func) -> any:
        self.imp.func = func
        return self.imp
