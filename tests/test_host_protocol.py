"""Wire tests use independent byte fixtures and malformed streams."""
import struct
import unittest
import zlib

try:
    from host.protocol import Frame, FrameParser, MessageType, Response, FirmwareImage
except ImportError:
    Frame = FrameParser = MessageType = Response = FirmwareImage = None


class ProtocolTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(Frame, 'host protocol must exist')

    def test_query_matches_c_fixture(self):
        self.assertEqual(Frame(1, 0, 0).encode().hex(),
                         '47424f54010100000000000000004cf9375a')

    def test_split_junk_overlap_and_crc_resynchronize(self):
        good = bytes.fromhex('47424f54010100000000000000004cf9375a')
        bad = good[:-1] + b'\x00'
        oversized = b'GBOT' + struct.pack('<BBIHH', 1, 1, 0, 0, 133)
        parser = FrameParser()
        frames = []
        for byte in b'x' * 10000 + b'G' + bad + oversized + good:
            frames.extend(parser.feed(bytes([byte])))
            self.assertLessEqual(parser.buffered_bytes, 150)
        self.assertEqual(len(frames), 1)
        self.assertEqual(frames[0].type, 1)
        self.assertEqual(frames[0].session, 0)

    def test_payload_limit_and_unknown_wire_type(self):
        with self.assertRaises(ValueError):
            Frame(3, 1, 1, b'0' * 133).encode()
        frame = Frame(99, 7, 4, b'x')
        self.assertEqual(FrameParser().feed(frame.encode()), [frame])

    def test_invalid_version_is_discarded_as_soon_as_available(self):
        parser = FrameParser()
        self.assertEqual(parser.feed(b'GBOT\x02'), [])
        self.assertEqual(parser.buffered_bytes, 0)

    def test_response_exact_length_state_and_error_validation(self):
        for payload in (b'', b'0' * 27, b'0' * 29,
                        struct.pack('<7I', 11, 0, 1, 0, 1, 257, 1),
                        struct.pack('<7I', 0, 0, 4, 0, 1, 257, 1)):
            with self.assertRaises(ValueError):
                Response.from_frame(Frame(6, 7, 1, payload))
        response = Response.from_frame(Frame(6, 7, 1, struct.pack(
            '<7I', 0, 0, 0x80000001, 1, 1, 257, 0x6B3D46C8)))
        self.assertTrue(response.application)
        self.assertEqual(response.state, 1)

    def test_bin_limits_and_vectors(self):
        data = struct.pack('<II', 0x20005000, 0x08004009) + bytes(381)
        image = FirmwareImage(data, 2)
        self.assertEqual(image.length, 389)
        self.assertEqual(image.crc, zlib.crc32(data))
        for size in (0, 7, 23553):
            with self.assertRaises(ValueError):
                FirmwareImage(bytes(size), 2)
        for msp, reset in ((0x20000000, 0x08004009), (0x20005008, 0x08004009),
                           (0x20004FFC, 0x08004009), (0x20005000, 0x08004008),
                           (0x20005000, 0x08000009), (0x20005000, 0x08004187)):
            with self.assertRaises(ValueError):
                FirmwareImage(struct.pack('<II', msp, reset) + bytes(381), 2)
        for version in (0, -1, 0x100000000, True, 1.5):
            with self.assertRaises(ValueError):
                FirmwareImage(data, version)


if __name__ == '__main__':
    unittest.main()
