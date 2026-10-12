"""Bounded GBOT v1 codec, shared by the CLI, GUI and uploader."""
from dataclasses import dataclass
from enum import IntEnum
import struct
import zlib

MAGIC = b'GBOT'
MAX_PAYLOAD = 132
MAX_FRAME = 150
APP_BASE = 0x08004000
APP_SIZE = 23552
MCU_ID = 0x103C8
APPLICATION_FLAG = 0x80000000
HEADER = struct.Struct('<4sBBIHH')


class MessageType(IntEnum):
    QUERY = 1
    BEGIN = 2
    DATA = 3
    COMMIT = 4
    CANCEL = 5
    ACK = 6
    NACK = 7
    DONE = 8


@dataclass(frozen=True)
class Frame:
    type: int
    session: int
    sequence: int
    payload: bytes = b''

    def encode(self):
        if len(self.payload) > MAX_PAYLOAD:
            raise ValueError('帧负载超过 132 字节')
        header = HEADER.pack(MAGIC, 1, self.type, self.session,
                             self.sequence, len(self.payload))
        body = header + self.payload
        return body + struct.pack('<I', zlib.crc32(body))


class FrameParser:
    """Matches the C parser's one-byte resynchronization and 150-byte bound."""
    def __init__(self):
        self._buffer = bytearray()

    @property
    def buffered_bytes(self):
        return len(self._buffer)

    def feed(self, data):
        frames = []
        for byte in data:
            self._buffer.append(byte)
            while self._buffer:
                prefix = min(len(self._buffer), 4)
                if self._buffer[:prefix] != MAGIC[:prefix]:
                    del self._buffer[0]
                    continue
                if len(self._buffer) >= 5 and self._buffer[4] != 1:
                    del self._buffer[0]
                    continue
                if len(self._buffer) < HEADER.size:
                    break
                _, version, kind, session, sequence, length = HEADER.unpack_from(self._buffer)
                if length > MAX_PAYLOAD:
                    del self._buffer[0]
                    continue
                size = HEADER.size + length + 4
                if len(self._buffer) < size:
                    break
                body = self._buffer[:size - 4]
                crc = struct.unpack_from('<I', self._buffer, size - 4)[0]
                if zlib.crc32(body) != crc:
                    del self._buffer[0]
                    continue
                frames.append(Frame(kind, session, sequence, bytes(self._buffer[14:size - 4])))
                del self._buffer[:size]
        return frames


@dataclass(frozen=True)
class Response:
    error: int
    expected_offset: int
    state: int
    request: int
    version: int
    length: int
    crc: int
    application: bool = False

    @classmethod
    def from_frame(cls, frame):
        if frame.type not in (MessageType.ACK, MessageType.NACK, MessageType.DONE):
            raise ValueError('不是响应帧')
        if len(frame.payload) != 28:
            raise ValueError('响应负载必须为 28 字节')
        error, offset, info, request, version, length, crc = struct.unpack('<7I', frame.payload)
        state = info & ~APPLICATION_FLAG
        if error > 10 or state > 3 or request > 1 or offset > APP_SIZE or length > APP_SIZE:
            raise ValueError('响应字段超出范围')
        return cls(error, offset, state, request, version, length, crc,
                   bool(info & APPLICATION_FLAG))


@dataclass(frozen=True)
class FirmwareImage:
    data: bytes
    version: int

    def __post_init__(self):
        object.__setattr__(self, 'data', bytes(self.data))
        if type(self.version) is not int or not 1 <= self.version <= 0xFFFFFFFF:
            raise ValueError('固件版本必须是 1 至 4294967295 的整数')
        if not 8 <= len(self.data) <= APP_SIZE:
            raise ValueError('BIN 长度必须为 8 至 23552 字节')
        msp, reset = struct.unpack_from('<II', self.data)
        if not 0x20000000 < msp <= 0x20005000 or msp % 8:
            raise ValueError('BIN 初始栈指针无效：需要 RAM 范围内的 8 字节对齐地址')
        if not reset & 1 or not APP_BASE <= (reset & ~1) < APP_BASE + len(self.data):
            raise ValueError('BIN 复位向量无效：请使用链接到 0x08004000 的应用固件')

    @property
    def length(self):
        return len(self.data)

    @property
    def crc(self):
        return zlib.crc32(self.data)

    def begin_payload(self):
        return struct.pack('<5I', MCU_ID, APP_BASE, self.length, self.crc, self.version)
