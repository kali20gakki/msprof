class ProfBean:
    def __init__(self: any) -> None:
        pass

    def get_attr(self: any) -> dict:
        return self.__dict__

    def get_name(self: any) -> str:
        return self.__name__