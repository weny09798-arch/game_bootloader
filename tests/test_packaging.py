import importlib.util
import pathlib
import struct
import unittest
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[1]


class PackagingTests(unittest.TestCase):
    def tool(self):
        path = ROOT / 'tools' / 'packaging.py'
        self.assertTrue(path.exists(), 'firmware packaging tool is missing')
        spec = importlib.util.spec_from_file_location('firmware_packaging', path)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        return mod

    def image(self, length=257):
        return struct.pack('<II', 0x20005000, 0x08004009) + bytes((i % 251 for i in range(length - 8)))

    def test_factory_image_has_committed_crc_record(self):
        p = self.tool()
        app = self.image()
        records = p.factory_segments(b'boot', app, 1)
        record = records[0x0800F800]
        self.assertEqual(len(record), 64)
        self.assertEqual(struct.unpack_from('<IIII', record), (0x4F54414D, 1, 1, 0))
        self.assertEqual(struct.unpack_from('<III', record, 16), (257, zlib.crc32(app), 1))
        self.assertEqual(struct.unpack_from('<I', record, 56)[0], zlib.crc32(record[:56]))
        self.assertEqual(record[60:], b'\x5a\xa5\xff\xff')
        text = p.intel_hex(records)
        self.assertTrue(text.endswith(':00000001FF\n'))
        for line in text.splitlines():
            self.assertEqual(sum(bytes.fromhex(line[1:])) % 256, 0)

    def test_out_of_partition_images_are_rejected(self):
        p = self.tool()
        with self.assertRaises(ValueError):
            p.factory_segments(b'x' * 0x4001, self.image(), 1)
        with self.assertRaises(ValueError):
            p.validate_app(self.image(0x5c01))

    def test_old_link_address_and_bad_stack_are_rejected(self):
        p = self.tool()
        for stack, reset in ((0x20000000, 0x08004009), (0x20005008, 0x08004009),
                             (0x20004fff, 0x08004009), (0x20005000, 0x08000009),
                             (0x20005000, 0x08004008), (0x20005000, 0x08009c01)):
            with self.subTest(stack=stack, reset=reset), self.assertRaises(ValueError):
                p.validate_app(struct.pack('<II', stack, reset) + b'12345678')


if __name__ == '__main__':
    unittest.main()
