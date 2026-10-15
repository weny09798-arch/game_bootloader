"""Exercise real OLED init/graphics and game drawing with a RAM controller model."""
import os
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]


class OledDisplayTests(unittest.TestCase):
    def test_cold_edge_ram_and_game_text_do_not_overlap(self):
        compiler = os.environ.get('HOST_CC') or shutil.which('gcc')
        self.assertTrue(compiler, 'GCC required for actual OLED/game regression')
        folder = ROOT / 'build' / 'native' / 'oled'
        folder.mkdir(parents=True, exist_ok=True)
        includes = ['-Itests/oled_stubs', '-Ifirmware/legacy/Inc', '-Ifirmware/app']
        flags = ['-std=c99', '-O1', '-fno-inline', '-ffunction-sections', '-fdata-sections']
        driver = folder / 'oled.o'
        subprocess.run([compiler, *flags, *includes, '-include', 'tests/oled_stubs/weak_bus.h', '-c',
                        'firmware/legacy/Src/OLED_I2C.c', '-o', str(driver)], cwd=ROOT, check=True)
        # Substitute only the external bus/GPIO boundary. All init, graphics,
        # font rendering and game draw functions remain production code.
        for version in (1, 2):
            with self.subTest(version=version):
                game = folder / f'game_v{version}.o'
                subprocess.run([compiler, *flags, *includes, f'-DGAME_VERSION={version}',
                                '-Dmain=game_firmware_main', '-c', 'firmware/app/game.c',
                                '-o', str(game)], cwd=ROOT, check=True)
                exe = folder / (f'v{version}.exe' if os.name == 'nt' else f'v{version}')
                subprocess.run([compiler, *flags, *includes, f'-DGAME_VERSION={version}',
                                str(driver), str(game), 'tests/native/oled_display_test.c',
                                'firmware/legacy/Src/OLED_Data.c', 'firmware/app/characters.c',
                                '-Wl,--gc-sections', '-lm', '-o', str(exe)], cwd=ROOT, check=True)
                result = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
