"""Actual C flash engine, with faults only at the transport boundary."""
import os
from pathlib import Path
import struct
import subprocess
import threading
import time
import unittest
import zlib

try:
    from host.protocol import Frame, FrameParser, FirmwareImage, Response, MessageType
    from host.uploader import Uploader, UploadError, RetryExhausted, Cancelled, UnknownOutcome
except ImportError:
    Uploader = None

ROOT = Path(__file__).resolve().parents[1]
NATIVE = ROOT / 'build' / 'native' / ('ota_native.exe' if os.name == 'nt' else 'ota_native')


def image_bytes(size=389):
    return struct.pack('<II', 0x20005000, 0x08004009) + bytes(
        (i * 17 + 3) & 255 for i in range(8, size))


class NativeTransport:
    """Test-only serial adapter. C subprocess owns all device state."""
    timeout = 0.001

    def __init__(self, fault=None, partial_write=None):
        self.process = subprocess.Popen([str(NATIVE), 'serve'], stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.fault = fault
        self.partial_write = partial_write
        self.outgoing = bytearray()
        self.pending = bytearray()
        self.requests = []
        self.counts = {}
        self.previous = b''
        self.closed = False

    def write(self, packet):
        if self.closed:
            raise OSError('closed')
        count = min(len(packet), self.partial_write or len(packet))
        self.outgoing.extend(packet[:count])
        if len(self.outgoing) >= 14:
            length = struct.unpack_from('<H', self.outgoing, 12)[0] + 18
            if len(self.outgoing) == length:
                wire = bytes(self.outgoing)
                self.outgoing.clear()
                self.requests.append(wire)
                kind = wire[5]
                self.counts[kind] = self.counts.get(kind, 0) + 1
                self.process.stdin.write(wire.hex().encode('ascii') + b'\n')
                self.process.stdin.flush()
                reply = bytes.fromhex(self.process.stdout.readline().decode('ascii').strip())
                if self.fault:
                    reply = self.fault(self, wire, reply)
                self.pending.extend(reply)
                if reply:
                    self.previous = reply
        return count

    def read(self, size):
        if self.closed:
            raise OSError('closed')
        if not self.pending:
            time.sleep(self.timeout)
            return b''
        result = bytes(self.pending[:min(size, 7)])
        del self.pending[:len(result)]
        return result

    def close(self):
        if not self.closed:
            self.closed = True
            self.process.stdin.close()
            self.process.terminate()
            self.process.wait(timeout=5)
            self.process.stdout.close()
            self.process.stderr.close()


def change_reply(reply, *, kind=None, session=None, length=None, offset=None,
                 state=None, version=None, error=None, crc=None):
    parser = FrameParser()
    frame = parser.feed(reply)[0]
    words = list(struct.unpack('<7I', frame.payload))
    for index, value in ((0, error), (1, offset), (2, state), (4, version),
                         (5, length), (6, crc)):
        if value is not None:
            words[index] = value
    return Frame(kind or frame.type, frame.session if session is None else session,
                 frame.sequence, struct.pack('<7I', *words)).encode()


class UploaderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not NATIVE.exists():
            from tools.verify import native_build
            native_build()

    def setUp(self):
        self.assertIsNotNone(Uploader, 'host uploader must exist')
        self.transports = []

    def tearDown(self):
        for transport in self.transports:
            transport.close()

    def uploader(self, fault=None, partial_write=None, **options):
        transport = NativeTransport(fault, partial_write)
        self.transports.append(transport)
        uploader = Uploader(transport, response_timeout=0.015, control_timeout=0.015,
                            **options)
        return uploader, transport

    def test_real_c_389_byte_v2_upload_then_query(self):
        uploader, transport = self.uploader(partial_write=3)
        progress = []
        image = FirmwareImage(image_bytes(), 2)
        result = uploader.upload(image, progress=lambda done, total: progress.append(done))
        self.assertEqual((result.version, result.length, result.crc),
                         (2, 389, zlib.crc32(image_bytes())))
        self.assertEqual(progress, [0, 128, 256, 384, 389])
        self.assertEqual(transport.requests[-1][5], 1)
        info = uploader.query()
        self.assertEqual((info.state, info.request, info.version, info.length), (1, 0, 2, 389))

    def test_odd_and_maximum_image_sizes(self):
        for size in (9, 23551, 23552):
            with self.subTest(size=size):
                uploader, _ = self.uploader()
                result = uploader.upload(FirmwareImage(image_bytes(size), 2))
                self.assertEqual(result.length, size)

    def test_lost_data_ack_retries_identical_frame_and_ignores_stale(self):
        def fault(t, packet, reply):
            if packet[5] == 3 and t.counts[3] == 1:
                return b''
            if packet[5] == 3:
                return t.previous + reply
            return reply
        uploader, transport = self.uploader(fault)
        result = uploader.upload(FirmwareImage(image_bytes(), 2))
        data = [packet for packet in transport.requests if packet[5] == 3]
        self.assertEqual(data[0], data[1])
        self.assertEqual(result.version, 2)

    def test_bad_crc_short_payload_wrong_session_and_wrong_offset_retry(self):
        def fault(t, packet, reply):
            if packet[5] == 3 and t.counts[3] == 1:
                frame = FrameParser().feed(reply)[0]
                short = Frame(frame.type, frame.session, frame.sequence, b'0' * 27).encode()
                bad = reply[:-1] + bytes([reply[-1] ^ 1])
                wrong_session = change_reply(reply, session=frame.session ^ 1)
                wrong_offset = change_reply(reply, offset=999999)
                return bad + short + wrong_session + wrong_offset
            return reply
        uploader, transport = self.uploader(fault)
        progress = []
        uploader.upload(FirmwareImage(image_bytes(), 2),
                        progress=lambda done, total: progress.append(done))
        self.assertEqual(progress, [0, 128, 256, 384, 389])
        self.assertEqual(transport.counts[3], 5)

    def test_bounded_wrong_offset_and_wrong_sequence_do_not_advance_progress(self):
        def fault(t, packet, reply):
            if packet[5] == 3 and t.counts[3] == 1:
                frame = FrameParser().feed(reply)[0]
                wrong_sequence = Frame(frame.type, frame.session, frame.sequence + 1,
                                       frame.payload).encode()
                return change_reply(reply, offset=127) + wrong_sequence
            return reply
        uploader, transport = self.uploader(fault)
        progress = []
        uploader.upload(FirmwareImage(image_bytes(), 2),
                        progress=lambda done, total: progress.append(done))
        self.assertEqual(progress, [0, 128, 256, 384, 389])
        self.assertEqual(transport.counts[3], 5)

    def test_zero_write_is_finite_and_no_device_erase(self):
        uploader, transport = self.uploader()
        transport.write = lambda packet: 0
        with self.assertRaisesRegex(UploadError, '写入'):
            uploader.upload(FirmwareImage(image_bytes(), 2))
        self.assertEqual(transport.requests, [])

    def test_unbounded_transport_timeout_is_rejected(self):
        transport = NativeTransport()
        self.transports.append(transport)
        transport.timeout = None
        with self.assertRaises(ValueError):
            Uploader(transport)

    def test_retry_exhaustion_six_identical_attempts(self):
        def fault(t, packet, reply):
            return b'' if packet[5] == 3 else reply
        uploader, transport = self.uploader(fault)
        with self.assertRaises(RetryExhausted):
            uploader.upload(FirmwareImage(image_bytes(), 2))
        packets = [p for p in transport.requests if p[5] == 3]
        self.assertEqual(len(packets), 6)
        self.assertEqual(len(set(packets)), 1)
        self.assertEqual(uploader.query().version, 1)

    def test_application_flag_blocks_begin_with_manual_reset_guidance(self):
        def fault(t, packet, reply):
            return change_reply(reply, state=0x80000001)
        uploader, transport = self.uploader(fault)
        with self.assertRaisesRegex(UploadError, 'KEY|按键'):
            uploader.upload(FirmwareImage(image_bytes(), 2))
        self.assertNotIn(2, transport.counts)

    def test_cancel_after_begin_preserves_original_application(self):
        event = threading.Event()
        def fault(t, packet, reply):
            if packet[5] == 2:
                event.set()
            return reply
        uploader, transport = self.uploader(fault, cancel_event=event)
        with self.assertRaises(Cancelled):
            uploader.upload(FirmwareImage(image_bytes(), 2))
        self.assertEqual(uploader.query().version, 1)
        self.assertIn(5, transport.counts)
        self.assertNotIn(4, transport.counts)

    def test_cancel_after_lost_data_ack_uses_device_offset(self):
        event = threading.Event()
        def fault(t, packet, reply):
            if packet[5] == 3:
                event.set()
                return b''
            return reply
        uploader, transport = self.uploader(fault, cancel_event=event)
        with self.assertRaises(Cancelled):
            uploader.upload(FirmwareImage(image_bytes(), 2))
        self.assertEqual(uploader.query().version, 1)
        cancel = next(p for p in transport.requests if p[5] == 5)
        self.assertEqual(struct.unpack_from('<H', cancel, 10)[0], 2)

    def test_lost_cancel_ack_reports_unknown_even_if_device_cancelled(self):
        event = threading.Event()
        def fault(t, packet, reply):
            if packet[5] == 2:
                event.set()
            if packet[5] == 5:
                return b''
            return reply
        uploader, _ = self.uploader(fault, cancel_event=event)
        with self.assertRaises(UnknownOutcome):
            uploader.upload(FirmwareImage(image_bytes(), 2))
        self.assertEqual(uploader.phase, 'unknown')

    def test_cancel_during_commit_waits_for_verified_success(self):
        event = threading.Event()
        def fault(t, packet, reply):
            if packet[5] == 4:
                event.set()
            return reply
        uploader, transport = self.uploader(fault, cancel_event=event)
        result = uploader.upload(FirmwareImage(image_bytes(), 2))
        self.assertEqual(result.version, 2)
        self.assertNotIn(5, transport.counts)

    def test_lost_done_retry_and_query_confirms_committed_image(self):
        def fault(t, packet, reply):
            return b'' if packet[5] == 4 else reply
        uploader, transport = self.uploader(fault)
        result = uploader.upload(FirmwareImage(image_bytes(), 2))
        self.assertEqual(result.version, 2)
        commits = [p for p in transport.requests if p[5] == 4]
        self.assertEqual(len(commits), 6)
        self.assertEqual(len(set(commits)), 1)

    def test_commit_reboot_session_loss_query_can_confirm(self):
        def fault(t, packet, reply):
            if packet[5] == 4:
                return change_reply(reply, kind=7, error=7, state=0x80000001)
            if packet[5] == 1 and t.counts[1] > 1:
                return change_reply(reply, state=0x80000001)
            return reply
        uploader, _ = self.uploader(fault)
        self.assertEqual(uploader.upload(FirmwareImage(image_bytes(), 2)).version, 2)

    def test_wrong_done_metadata_and_post_query_metadata_never_succeed(self):
        for field in ('version', 'crc', 'length', 'state'):
            with self.subTest(field=field):
                def fault(t, packet, reply):
                    if packet[5] == 4 or (packet[5] == 1 and t.counts[1] > 1):
                        return change_reply(reply, **{field: 3})
                    return reply
                uploader, _ = self.uploader(fault)
                with self.assertRaises(UnknownOutcome):
                    uploader.upload(FirmwareImage(image_bytes(), 2))

    def test_valid_done_does_not_override_wrong_post_install_query(self):
        def fault(t, packet, reply):
            if packet[5] == 1 and t.counts[1] > 1:
                return change_reply(reply, version=99)
            return reply
        uploader, _ = self.uploader(fault)
        with self.assertRaises(UnknownOutcome):
            uploader.upload(FirmwareImage(image_bytes(), 2))

    def test_closed_transport_in_commit_window_reports_unknown(self):
        def fault(t, packet, reply):
            if packet[5] == 4:
                t.close()
                raise OSError('link disappeared during installation')
            return reply
        uploader, _ = self.uploader(fault)
        with self.assertRaises(UnknownOutcome):
            uploader.upload(FirmwareImage(image_bytes(), 2))

    def test_done_ack_is_not_commit_success_and_failed_query_is_unknown(self):
        def fault(t, packet, reply):
            if packet[5] == 4:
                return change_reply(reply, kind=6)
            if packet[5] == 1 and t.counts[1] > 1:
                return b''
            return reply
        uploader, _ = self.uploader(fault)
        with self.assertRaises(UnknownOutcome):
            uploader.upload(FirmwareImage(image_bytes(), 2))

    def test_factory_release_if_present(self):
        path = ROOT / 'dist' / 'dinosaur.bin'
        if not path.exists():
            self.skipTest('factory release is not generated yet')
        uploader, _ = self.uploader()
        image = FirmwareImage(path.read_bytes(), 2)
        self.assertEqual(uploader.upload(image).crc, zlib.crc32(path.read_bytes()))

    def test_keyboard_interrupt_during_receive_cancels_device(self):
        def fault(t, packet, reply):
            if packet[5] == 3:
                raise KeyboardInterrupt()
            return reply
        uploader, _ = self.uploader(fault)
        try:
            with self.assertRaises(Cancelled):
                uploader.upload(FirmwareImage(image_bytes(), 2))
        except KeyboardInterrupt:
            self.fail('receive interruption must request confirmed device cancellation')
        self.assertEqual(uploader.query().version, 1)

    def test_keyboard_interrupt_during_commit_still_confirms_result(self):
        def fault(t, packet, reply):
            if packet[5] == 4:
                raise KeyboardInterrupt()
            return reply
        uploader, _ = self.uploader(fault)
        try:
            self.assertEqual(uploader.upload(FirmwareImage(image_bytes(), 2)).version, 2)
        except KeyboardInterrupt:
            self.fail('commit interruption must query instead of abandoning confirmation')


if __name__ == '__main__':
    unittest.main()
