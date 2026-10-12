"""Generate complete uVision5 settings from the user's working project template."""
import argparse
import copy
from pathlib import Path
import xml.etree.ElementTree as ET
from build import ROOT, sources, INCLUDES


def set_text(parent, path, value):
    child = parent.find(path)
    if child is None:
        raise ValueError('Template missing ' + path)
    child.text = str(value)


def generate(template):
    tree = ET.parse(template)
    target_parent = tree.getroot().find('Targets')
    original = copy.deepcopy(target_parent.find('Target'))
    for child in list(target_parent):
        target_parent.remove(child)
    all_files = list(dict.fromkeys(sources('boot') + sources('deer')))
    for name in ('boot', 'deer', 'dinosaur'):
        target = copy.deepcopy(original)
        set_text(target, 'TargetName', name)
        common = target.find('TargetOption/TargetCommonOption')
        base = '0x08000000' if name == 'boot' else '0x08004000'
        size = '0x4000' if name == 'boot' else '0x5c00'
        set_text(common, 'Cpu', f'IRAM(0x20000000,0x5000) IROM({base},{size}) CPUTYPE("Cortex-M3") CLOCK(72000000) ELITTLE')
        set_text(common, 'OutputDirectory', '.\\build\\keil-' + name + '\\')
        set_text(common, 'OutputName', name)
        set_text(common, 'ListingPath', '.\\build\\keil-' + name + '\\')
        set_text(common, 'CreateExecutable', 1)
        set_text(common, 'CreateHexFile', 1)
        arm = target.find('TargetOption/TargetArmAds')
        for field in ('IROM', 'OCR_RVCT4'):
            set_text(arm, 'ArmAdsMisc/OnChipMemories/' + field + '/StartAddress', base)
            set_text(arm, 'ArmAdsMisc/OnChipMemories/' + field + '/Size', size)
        set_text(arm, 'Cads/Optim', 2)
        set_text(arm, 'Cads/SplitLS', 0)
        set_text(arm, 'Cads/OneElfS', 1)
        control = arm.find('Cads/VariousControls')
        defines = 'USE_STDPERIPH_DRIVER,STM32F10X_MD,VECT_TAB_OFFSET=' + ('0' if name == 'boot' else '0x4000')
        defines += ',OTA_BOOTLOADER' if name == 'boot' else ',OTA_APPLICATION,GAME_VERSION=' + ('1' if name == 'deer' else '2')
        set_text(control, 'Define', defines)
        set_text(control, 'IncludePath', ';'.join('.\\' + p.replace('/', '\\') for p in INCLUDES))
        set_text(control, 'MiscControls', '--diag_suppress=1,1295')
        set_text(arm, 'LDads/TextAddressRange', base)
        set_text(arm, 'LDads/DataAddressRange', '0x20000000')
        groups = target.find('Groups')
        for child in list(groups):
            groups.remove(child)
        group = ET.SubElement(groups, 'Group')
        ET.SubElement(group, 'GroupName').text = 'Firmware'
        files = ET.SubElement(group, 'Files')
        included = set(sources(name))
        for path in all_files:
            file = ET.SubElement(files, 'File')
            ET.SubElement(file, 'FileName').text = Path(path).name
            ET.SubElement(file, 'FileType').text = '2' if path.endswith('.s') else '1'
            ET.SubElement(file, 'FilePath').text = '.\\' + path.replace('/', '\\')
            options = ET.SubElement(file, 'FileOption')
            props = ET.SubElement(options, 'CommonProperty')
            ET.SubElement(props, 'UseCPPCompiler').text = '0'
            ET.SubElement(props, 'RVCTCodeConst').text = '0'
            ET.SubElement(props, 'RVCTZI').text = '0'
            ET.SubElement(props, 'RVCTOtherData').text = '0'
            ET.SubElement(props, 'ModuleSelection').text = '0'
            ET.SubElement(props, 'IncludeInBuild').text = '1' if path in included else '0'
            ET.SubElement(props, 'AlwaysBuild').text = '0'
            ET.SubElement(props, 'GenerateAssemblyFile').text = '0'
            ET.SubElement(props, 'AssembleAssemblyFile').text = '0'
            ET.SubElement(props, 'PublicsOnly').text = '0'
            ET.SubElement(props, 'StopOnExitCode').text = '3'
            ET.SubElement(props, 'CustomArgument').text = ''
            ET.SubElement(props, 'IncludeLibraryModules').text = ''
        target_parent.append(target)
    ET.indent(tree, space='  ')
    return '<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n' + ET.tostring(tree.getroot(), encoding='unicode')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('template', type=Path)
    args = parser.parse_args()
    print(generate(args.template))
