"""Validate fixed-address app images and create checksummed first-flash HEX."""
import struct
import zlib

APP_BASE = 0x08004000
APP_SIZE = 0x5C00


def validate_app(image):
    if not 8 <= len(image) <= APP_SIZE:
        raise ValueError('Application image must be 8..23552 bytes')
    stack, reset = struct.unpack_from('<II', image)
    if not (0x20000000 < stack <= 0x20005000 and stack % 8 == 0):
        raise ValueError('Invalid C8T6 initial stack pointer')
    if not (reset & 1 and APP_BASE <= (reset & ~1) < APP_BASE + len(image)):
        raise ValueError('Image is not linked for A region 0x08004000')


def factory_segments(boot, app, version):
    if not 0 < len(boot) <= 0x4000:
        raise ValueError('Bootloader exceeds B region')
    validate_app(app)
    if not 1 <= version <= 0xFFFFFFFF:
        raise ValueError('Version must be a positive uint32')
    record = struct.pack('<14I', 0x4F54414D, 1, 1, 0,
                         len(app), zlib.crc32(app), version, 0, 0, 0, 0, 0, 0, 0)
    record += struct.pack('<IHH', zlib.crc32(record), 0xA55A, 0xFFFF)
    return {0x08000000: boot, APP_BASE: app, 0x0800F800: record}


def intel_hex(segments):
    lines = []
    upper = None

    def emit(kind, address, data):
        raw = bytes((len(data), address >> 8, address & 255, kind)) + data
        lines.append(':' + (raw + bytes((-sum(raw) & 255,))).hex().upper())

    previous_end = 0
    for base, image in sorted(segments.items()):
        if base < previous_end:
            raise ValueError('Overlapping HEX segments')
        previous_end = base + len(image)
        for offset in range(0, len(image), 16):
            address = base + offset
            if address >> 16 != upper:
                upper = address >> 16
                emit(4, 0, struct.pack('>H', upper))
            emit(0, address & 65535, image[offset:offset + 16])
    emit(1, 0, b'')
    return '\n'.join(lines) + '\n'
