"""Build and execute the actual portable C engine and Python tests."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def native_build(gcc=None):
    compiler = gcc or os.environ.get('HOST_CC') or shutil.which('gcc')
    if not compiler:
        raise RuntimeError('GCC required for native firmware validation; set HOST_CC')
    folder = ROOT / 'build' / 'native'
    folder.mkdir(parents=True, exist_ok=True)
    output = folder / ('ota_native.exe' if os.name == 'nt' else 'ota_native')
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror', '-pedantic', '-O1',
                    '-Ifirmware/common', 'firmware/common/ota.c', 'firmware/common/protocol.c',
                    'tests/native/main.c', '-o', str(output)], cwd=ROOT, check=True)
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gcc')
    args = parser.parse_args()
    exe = native_build(args.gcc)
    subprocess.run([str(exe), 'test'], cwd=ROOT, check=True)
    compiler = args.gcc or os.environ.get('HOST_CC') or shutil.which('gcc')
    port_exe = ROOT / 'build' / 'native' / ('app_port_test.exe' if os.name == 'nt' else 'app_port_test')
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-Ifirmware/common', '-Ifirmware/app', '-Itests/stubs',
                    'firmware/common/ota.c', 'firmware/common/protocol.c',
                    'firmware/app/app_support.c', 'tests/native/app_port_test.c',
                    '-o', str(port_exe)], cwd=ROOT, check=True)
    subprocess.run([str(port_exe)], cwd=ROOT, check=True)
    debounce_exe = ROOT / 'build' / 'native' / ('debounce_test.exe' if os.name == 'nt' else 'debounce_test')
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror', '-pedantic', '-O1',
                    '-Ifirmware/port', 'firmware/port/debounce.c', 'tests/native/debounce_test.c',
                    '-o', str(debounce_exe)], cwd=ROOT, check=True)
    subprocess.run([str(debounce_exe)], cwd=ROOT, check=True)
    subprocess.run([sys.executable, '-m', 'unittest', 'discover', '-s', 'tests', '-v'], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
