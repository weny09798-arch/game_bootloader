"""Synchronous stop-and-wait uploader; UI callers run it in a worker thread."""
import secrets
import struct
import threading
import time

from .protocol import Frame, FrameParser, FirmwareImage, MessageType as T, Response


class UploadError(RuntimeError):
    pass


class RetryExhausted(UploadError):
    pass


class Cancelled(UploadError):
    """The device acknowledged cancellation before installation."""


class UnknownOutcome(UploadError):
    """The device might have changed; a fresh query is required."""


class _CancelSignal(Exception):
    pass


ERRORS = ('成功', '设备状态不允许', '长度错误', '芯片或地址错误', '向量错误',
          'CRC 错误', 'Flash 操作失败', '会话不匹配', '序号错误', '偏移错误', '内容不一致')


class Uploader:
    """Transport must implement read(size), write(bytes) and a finite timeout.

    Retries keep exactly the same packet/session/sequence. Progress reports
    acknowledged bytes. Cancellation is accepted only before COMMIT. This
    class does not own the transport; CLI/GUI callers close it in finally.
    """
    def __init__(self, transport, *, response_timeout=5.0, control_timeout=30.0,
                 max_retries=5, cancel_event=None, log=None):
        if response_timeout <= 0 or control_timeout <= 0:
            raise ValueError('超时必须大于零')
        if type(max_retries) is not int or max_retries < 0:
            raise ValueError('重试次数必须为非负整数')
        timeout = getattr(transport, 'timeout', None)
        if timeout is None or timeout < 0 or timeout > min(response_timeout, control_timeout):
            raise ValueError('串口读取必须设置有限超时，且不能超过响应超时')
        self.transport = transport
        self.response_timeout = response_timeout
        self.control_timeout = control_timeout
        self.max_retries = max_retries
        self.cancel_event = cancel_event or threading.Event()
        self.log = log or (lambda message: None)
        self.phase = 'idle'
        self._parser = FrameParser()
        self._session = 0
        self._image = None

    def request_cancel(self):
        self.cancel_event.set()

    def _write_all(self, packet, timeout):
        deadline = time.monotonic() + timeout
        sent = 0
        while sent < len(packet):
            if time.monotonic() >= deadline:
                raise UploadError('串口写入超时')
            count = self.transport.write(packet[sent:])
            if type(count) is not int or not 0 < count <= len(packet) - sent:
                raise UploadError('串口未完整写入数据')
            sent += count

    def _exchange(self, frame, *, timeout=None, accept=None, cancellable=False):
        packet = frame.encode()
        timeout = self.response_timeout if timeout is None else timeout
        for attempt in range(self.max_retries + 1):
            if cancellable and self.cancel_event.is_set():
                raise _CancelSignal()
            self._write_all(packet, timeout)
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                if cancellable and self.cancel_event.is_set():
                    raise _CancelSignal()
                data = self.transport.read(150)
                if len(data) > 150:
                    raise UploadError('串口读取超出请求长度')
                for reply in self._parser.feed(data):
                    if reply.session != frame.session or reply.sequence != frame.sequence:
                        continue
                    try:
                        response = Response.from_frame(reply)
                    except ValueError:
                        continue
                    if reply.type == T.NACK:
                        if response.error:
                            raise UploadError('设备拒绝：' + ERRORS[response.error])
                        continue
                    if response.error:
                        continue
                    if accept is None or accept(reply, response):
                        return response
                if not data:
                    time.sleep(min(0.001, max(0, deadline - time.monotonic())))
            if attempt < self.max_retries:
                self.log(f'等待响应超时，重试 {attempt + 1}/{self.max_retries}')
                # A plausible truncated frame must not swallow the retry's reply.
                self._parser = FrameParser()
        raise RetryExhausted(f'响应超时，已尝试 {self.max_retries + 1} 次')

    def query(self):
        return self._exchange(Frame(T.QUERY, secrets.randbits(32), 0),
                              accept=lambda frame, response: frame.type == T.ACK)

    @staticmethod
    def _matches(response, image):
        return (response.error == 0 and response.state == 1 and response.request == 0
                and (response.version, response.length, response.crc)
                == (image.version, image.length, image.crc))

    def _cancel_receive(self):
        """QUERY settles whether an in-flight DATA was consumed before CANCEL."""
        self.log('正在确认接收状态并请求取消；安装开始后不能回滚')
        try:
            info = self.query()
            if info.application or info.state != 2:
                raise UnknownOutcome('无法确认当前接收会话；请重新查询设备，未承诺回滚')
            offset = info.expected_offset
            if offset > self._image.length or (offset != self._image.length and offset % 128):
                raise UnknownOutcome('设备接收偏移异常；取消结果未知，请重新查询')
            sequence = 1 + (offset + 127) // 128
            self._exchange(Frame(T.CANCEL, self._session, sequence),
                           timeout=self.control_timeout,
                           accept=lambda frame, response: frame.type == T.ACK
                           and response.state in (0, 1) and not response.application)
        except UnknownOutcome:
            self.phase = 'unknown'
            raise
        except (UploadError, OSError) as error:
            self.phase = 'unknown'
            raise UnknownOutcome('未收到可靠取消确认；请重新查询设备，取消结果未知') from error
        self.phase = 'cancelled'
        raise Cancelled('设备已确认取消接收；安装未开始')

    def upload(self, image, *, progress=None):
        if not isinstance(image, FirmwareImage):
            raise TypeError('image 必须是经过校验的 FirmwareImage')
        if self.phase in ('receiving', 'committing'):
            raise UploadError('已有上传正在执行')
        progress = progress or (lambda done, total: None)
        self._image = image
        self.phase = 'querying'
        info = self.query()
        if info.application:
            self.phase = 'idle'
            raise UploadError('设备正在运行应用：请按 KEY 保存升级请求，再手动复位进入 Bootloader')
        if info.state == 3:
            self.phase = 'idle'
            raise UploadError('设备正在安装或恢复安装，请等待并重新查询')
        if self.cancel_event.is_set():
            self.phase = 'cancelled'
            raise Cancelled('已取消，尚未发送 BEGIN')
        self._session = secrets.randbits(32) or 1
        self.phase = 'receiving'
        try:
            self._exchange(Frame(T.BEGIN, self._session, 0, image.begin_payload()),
                           timeout=self.control_timeout, cancellable=True,
                           accept=lambda frame, response: frame.type == T.ACK
                           and response.state == 2 and not response.application
                           and response.expected_offset == 0)
            progress(0, image.length)
            sequence = 1
            for offset in range(0, image.length, 128):
                if self.cancel_event.is_set():
                    raise _CancelSignal()
                chunk = image.data[offset:offset + 128]
                end = offset + len(chunk)
                self._exchange(Frame(T.DATA, self._session, sequence,
                                     struct.pack('<I', offset) + chunk), cancellable=True,
                               accept=lambda frame, response: frame.type == T.ACK
                               and response.state == 2 and not response.application
                               and response.expected_offset == end)
                progress(end, image.length)
                sequence += 1
            if self.cancel_event.is_set():
                raise _CancelSignal()
        except (_CancelSignal, KeyboardInterrupt):
            self._cancel_receive()
        except (UploadError, OSError):
            self.phase = 'failed'
            raise
        self.phase = 'committing'
        self.log('数据已接收，正在安装；此阶段取消不能回滚，请等待最终查询确认')
        try:
            self._exchange(Frame(T.COMMIT, self._session, sequence),
                           timeout=self.control_timeout,
                           accept=lambda frame, response: frame.type == T.DONE
                           and self._matches(response, image))
        except (UploadError, OSError, KeyboardInterrupt) as error:
            self.log(f'安装响应未确认（{error}），重新查询设备')
        try:
            result = self.query()
        except (UploadError, OSError) as error:
            self.phase = 'unknown'
            raise UnknownOutcome('安装结果未知：无法查询设备，请重新连接并查询，勿假定成功或回滚') from error
        if not self._matches(result, image):
            self.phase = 'unknown'
            raise UnknownOutcome('安装结果未知：设备版本、长度、CRC 或状态不匹配，请重新查询')
        self.phase = 'complete'
        self.log('升级成功：设备查询已确认版本、长度与 CRC')
        return result
