"""Reproducible ARM Compiler 5 builds; source paths relative to repository."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import zlib

from packaging import factory_segments, intel_hex, validate_app

ROOT = Path(__file__).resolve().parents[1]
COMMON = ['firmware/common/ota.c', 'firmware/common/protocol.c']
PORT = ['firmware/port/board.c', 'firmware/port/jump.s']
START = ['firmware/legacy/Start/startup_stm32f10x_md.s',
         'firmware/legacy/Start/system_stm32f10x.c', 'firmware/legacy/Start/core_cm3.c']
LIBRARY = ['firmware/legacy/Library/' + n + '.c' for n in
           ('misc', 'stm32f10x_flash', 'stm32f10x_rcc', 'stm32f10x_gpio',
            'stm32f10x_usart', 'stm32f10x_exti', 'stm32f10x_tim')]
DISPLAY = ['firmware/legacy/Src/OLED_I2C.c', 'firmware/legacy/Src/OLED_Data.c']
INCLUDES = ['firmware/common', 'firmware/port', 'firmware/app',
            'firmware/legacy/Start', 'firmware/legacy/User', 'firmware/legacy/Library', 'firmware/legacy/Inc']


def sources(name):
    app = ['firmware/app/game.c', 'firmware/app/app_support.c', 'firmware/app/characters.c',
           'firmware/legacy/Src/generic.c']
    return COMMON + PORT + START + LIBRARY + DISPLAY + (['firmware/boot/main.c'] if name == 'boot' else app)


def run(command, log):
    result = subprocess.run([str(v) for v in command], cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    output = result.stdout.decode('utf-8', errors='replace')
    log.append('$ ' + ' '.join(str(v) for v in command) + '\n' + output)
    if result.returncode:
        print(output)
        raise RuntimeError('Build command failed with exit ' + str(result.returncode))
    if output.strip():
        print(output.strip())


def build_target(name, toolchain):
    folder = Path('build') / name
    (ROOT / folder).mkdir(parents=True, exist_ok=True)
    base = 0x08000000 if name == 'boot' else 0x08004000
    limit = 0x4000 if name == 'boot' else 0x5C00
    defines = ['USE_STDPERIPH_DRIVER', 'STM32F10X_MD',
               'VECT_TAB_OFFSET=' + hex(base - 0x08000000),
               'OTA_BOOTLOADER' if name == 'boot' else 'OTA_APPLICATION']
    if name != 'boot':
        defines.append('GAME_VERSION=' + ('1' if name == 'deer' else '2'))
    objects, log = [], []
    try:
        for index, source in enumerate(sources(name)):
            obj = folder / (str(index) + '_' + Path(source).stem + '.o')
            objects.append(obj)
            if source.endswith('.s'):
                command = [toolchain / 'armasm.exe', '--cpu', 'Cortex-M3', '-g', source, '-o', obj]
            else:
                command = [toolchain / 'armcc.exe', '--cpu', 'Cortex-M3', '--c99', '-O1',
                           '--split_sections', '--diag_suppress=1,1295', '-g']
                command += ['-D' + item for item in defines]
                command += ['-I' + item for item in INCLUDES]
                command += ['-c', source, '-o', obj]
            run(command, log)
        scatter = folder / (name + '.sct')
        (ROOT / scatter).write_text(
            f'LR_ROM {base:#010x} {limit:#010x} {{\n'
            f' ER_ROM {base:#010x} {limit:#010x} {{\n'
            '  *.o (RESET, +First)\n  *(InRoot$$Sections)\n  .ANY (+RO)\n }\n'
            ' RW_RAM 0x20000000 0x00005000 { .ANY (+RW +ZI) }\n}\n', encoding='ascii')
        axf = folder / (name + '.axf')
        run([toolchain / 'armlink.exe', '--scatter', scatter, '--strict', '--summary_stderr',
             '--map', '--list', folder / (name + '.map'), '-o', axf] + objects, log)
        run([toolchain / 'fromelf.exe', '--bin', '--output', folder / (name + '.bin'), axf], log)
        run([toolchain / 'fromelf.exe', '--i32', '--output', folder / (name + '.hex'), axf], log)
        image = (ROOT / folder / (name + '.bin')).read_bytes()
        if len(image) > limit:
            raise ValueError(name + ': image exceeds partition')
        if name != 'boot':
            validate_app(image)
        info = dict(target=name, base=hex(base), bytes=len(image), limit=limit,
                    crc32=f'{zlib.crc32(image):08X}', version=0 if name == 'boot' else (1 if name == 'deer' else 2))
        print(json.dumps(info))
        return image, info
    finally:
        (ROOT / folder / 'build.log').write_text('\n'.join(log), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--toolchain', default=os.environ.get('ARMCC5_BIN', 'D:/keil5/ARM/ARMCC/bin'))
    parser.add_argument('--target', choices=('all', 'boot', 'deer', 'dinosaur'), default='all')
    args = parser.parse_args()
    toolchain = Path(args.toolchain)
    for tool in ('armcc.exe', 'armasm.exe', 'armlink.exe', 'fromelf.exe'):
        if not (toolchain / tool).is_file():
            parser.error('Missing ' + str(toolchain / tool))
    names = ('boot', 'deer', 'dinosaur') if args.target == 'all' else (args.target,)
    built = {name: build_target(name, toolchain) for name in names}
    if args.target == 'all':
        release = ROOT / 'dist'
        release.mkdir(exist_ok=True)
        for name in names:
            for suffix in ('.bin', '.hex', '.axf', '.map'):
                shutil.copy2(ROOT / 'build' / name / (name + suffix), release)
        for game, version in (('deer', 1), ('dinosaur', 2)):
            (release / ('factory_' + game + '.hex')).write_text(
                intel_hex(factory_segments(built['boot'][0], built[game][0], version)), encoding='ascii')
        (release / 'manifest.json').write_text(json.dumps(
            {name: built[name][1] for name in names}, indent=2) + '\n', encoding='utf-8')
        print('Validated artifacts: ' + str(release))


if __name__ == '__main__':
    main()
