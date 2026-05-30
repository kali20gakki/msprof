import argparse
import logging

import cpu_binder

LOG_MAP = {0: logging.DEBUG, 1: logging.INFO, 2: logging.WARNING, 3: logging.ERROR}


def main():
    parser = argparse.ArgumentParser(description="总入口：支持多个子命令")
    subparsers = parser.add_subparsers(dest="command", required=True)

    bind_parser = subparsers.add_parser("bind", help="执行绑核逻辑")
    bind_parser.add_argument("-c", "--config", type=str, help="绑定进线程配置文件路径，自定义绑定推理进线程时使用")
    bind_parser.add_argument("-l", "--log-level", type=int, default=1, choices=[0, 1, 2, 3], help="日志级别")

    args = parser.parse_args()

    log_level = LOG_MAP.get(args.log_level, logging.INFO)
    logging.basicConfig(level=log_level, format='[%(asctime)s] [%(levelname)s]:%(message)s')

    if args.command == "bind":
        cpu_binder.run(args)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
