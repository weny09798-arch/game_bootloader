import importlib.util
import json
import pathlib
import struct
import sys
import tempfile
import unittest
from unittest import mock
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

    def test_published_version_aliases_match_images_and_manifest(self):
        spec = importlib.util.spec_from_file_location('firmware_build', ROOT / 'tools' / 'build.py')
        build = importlib.util.module_from_spec(spec)
        with mock.patch.dict(sys.modules, {'packaging': self.tool()}):
            spec.loader.exec_module(build)
        self.assertTrue(hasattr(build, 'publish_artifacts'),
                        'artifact publication must be usable without an ARM compiler')
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            images = {'boot': b'boot', 'deer': self.image(), 'dinosaur': self.image(513)}
            built = {}
            for name, version in (('boot', 0), ('deer', 1), ('dinosaur', 2)):
                folder = root / 'build' / name
                folder.mkdir(parents=True)
                for suffix in ('.bin', '.hex', '.axf', '.map'):
                    (folder / (name + suffix)).write_bytes(images[name])
                built[name] = (images[name], dict(target=name, version=version,
                                                bytes=len(images[name]),
                                                crc32=f'{zlib.crc32(images[name]):08X}'))
            with mock.patch.object(build, 'ROOT', root):
                build.publish_artifacts(built, root / 'dist')
            manifest = json.loads((root / 'dist' / 'manifest.json').read_text(encoding='utf-8'))
            for name, alias, version in (('deer', 'deer_v1.bin', 1),
                                          ('dinosaur', 'dinosaur_v2.bin', 2)):
                with self.subTest(name=name):
                    image = (root / 'dist' / alias).read_bytes()
                    self.assertEqual(image, images[name])
                    self.assertEqual(image, (root / 'dist' / (name + '.bin')).read_bytes())
                    self.assertEqual(manifest[name]['version'], version)
                    self.assertEqual(manifest[name]['bytes'], len(image))
                    self.assertEqual(manifest[name]['crc32'], f'{zlib.crc32(image):08X}')


if __name__ == '__main__':
    unittest.main()
