"""Run ``python -m host.cli --help`` for the serial upgrade commands."""
import argparse
from pathlib import Path
import re
import sys

from .protocol import FirmwareImage
from .uploader import Cancelled, UnknownOutcome, UploadError, Uploader


def infer_version(path):
    match = re.search(r'(?:^|[_-])v(\d+)(?:$|[_-])', Path(path).stem, re.IGNORECASE)
    if match:
        version = int(match.group(1))
        if 1 <= version <= 0xFFFFFFFF:
            return version
    return 1


def open_serial(port, **kwargs):
    try:
        import serial
    except ImportError as error:
        raise UploadError('缺少 pyserial，请先安装 requirements.txt 中的依赖') from error
    return serial.Serial(port, **kwargs)


def describe_info(info):
    states = ('空固件', '有效固件', '接收中', '安装中')
    mode = '应用程序' if info.application else 'Bootloader'
    return (f'设备：{mode}，{states[info.state]}，升级请求={info.request}，'
            f'版本={info.version}，长度={info.length}，CRC={info.crc:08X}，'
            f'已接收={info.expected_offset}')


def main(argv=None, *, serial_factory=None):
    parser = argparse.ArgumentParser(description='STM32 无线升级工具（GBOT）')
    parser.add_argument('--port', required=True, help='串口，例如 COM3')
    parser.add_argument('--baud', type=int, default=9600, help='波特率，默认 9600')
    parser.add_argument('--version', type=int, help='固件版本整数，默认从文件名 v1/v2 推断')
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('query', help='只查询设备状态')
    upgrade = commands.add_parser('upload', help='校验 BIN 后开始升级')
    upgrade.add_argument('bin', type=Path, help='链接到 0x08004000 的 BIN 文件')
    upgrade.add_argument('--version', type=int, default=argparse.SUPPRESS,
                         help='覆盖文件名推断的固件版本')
    args = parser.parse_args(argv)
    transport = None
    try:
        if args.baud <= 0:
            raise ValueError('波特率必须为正整数')
        image = None
        if args.command == 'upload':
            version = args.version if args.version is not None else infer_version(args.bin)
            image = FirmwareImage(args.bin.read_bytes(), version)
        transport = (serial_factory or open_serial)(
            args.port, baudrate=args.baud, timeout=0.1, write_timeout=5.0)
        uploader = Uploader(transport, log=print)
        if args.command == 'query':
            print(describe_info(uploader.query()))
        else:
            print(f'固件版本={image.version}，长度={image.length}，CRC={image.crc:08X}')
            result = uploader.upload(image, progress=lambda done, total:
                                     print(f'接收进度：{done}/{total} 字节'))
            print(describe_info(result))
        return 0
    except Cancelled as error:
        print(str(error), file=sys.stderr)
        return 2
    except UnknownOutcome as error:
        print(str(error), file=sys.stderr)
        return 3
    except (UploadError, OSError, ValueError) as error:
        print(f'操作失败：{error}', file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print('已中断；如已发送安装命令，请重新查询设备确认结果', file=sys.stderr)
        return 2
    finally:
        if transport is not None:
            try:
                transport.close()
            except OSError:
                pass


if __name__ == '__main__':
    raise SystemExit(main())
