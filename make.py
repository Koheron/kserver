#! /usr/bin/python

# (c) Koheron

import os
import sys
import yaml
import jinja2
import time
import subprocess
from shutil import copytree

from devgen import Device, device_table

TMP = 'tmp'
DEV_DIR = TMP + '/devices'
 
def render_makefile(obj_files):
    print "Generate Makefile"
    makefile_renderer = jinja2.Environment(
      loader = jinja2.FileSystemLoader(os.path.abspath('.'))
    )

    template = makefile_renderer.get_template('core/Makefile')
    date = time.strftime("%c")
    makefile_filename = os.path.join(TMP, 'Makefile')

    output = file(makefile_filename, 'w')
    output.write(template.render(date=date, objs_list=obj_files))
    output.close()

def render_device_table(devices):
    print "Generate device table"
    device_table.PrintDeviceTable(devices, DEV_DIR)
    template_filename = 'devgen/templates/devices.hpp'

    header_renderer = jinja2.Environment(
      loader = jinja2.FileSystemLoader(os.path.abspath('.'))
    )

    template = header_renderer.get_template(template_filename)
    header_filename = os.path.join(DEV_DIR, 'devices.hpp')
    output = file(header_filename, 'w')
    output.write(template.render(devices=devices))
    output.close()

def generate(devices_list):
    devices = [] # List of generated devices
    obj_files = []  # Object file names

    for path in devices_list:       
        device = Device(path)
        
        print "Generate " + device.name
        device.Generate(DEV_DIR)
        devices.append(device)
        obj_files.append(os.path.join('devices', device.class_name.lower()+'.o'))

    return devices, obj_files

def install_requirements(requirements):
    for requirement in requirements:
        if requirement['type'] == 'git':
            subprocess.call(['git', 'clone', requirement['src'], requirement['dest']])
            subprocess.call('cd ' + requirement['dest'] + ' && git checkout ' + requirement['branch'], shell=True)
        elif requirement['type'] == 'folder':
            copytree(os.path.join(requirement['from'], requirement['import']),
                     os.path.join('tmp/middleware', requirement['import']))

def main(argv):
    cmd = argv[0]

    tmp_dir = 'tmp'
    if not os.path.exists(tmp_dir):
        os.makedirs(tmp_dir)

    with open(argv[1]) as config_file:
        config = yaml.load(config_file)

    if cmd == '--generate':
        devices, obj_files = generate(config["devices"])
        render_makefile(obj_files)
        render_device_table(devices)

    elif cmd == '--host':
        with open('tmp/.host', 'w') as f:
            f.write(config['host'])

    elif cmd == '--devices':
        cpp_files = []
        for dev in config['devices']:
            cpp_filename = os.path.join(os.path.dirname(dev), 
                                        os.path.basename(dev).split('.')[0] + '.cpp')
            if os.path.exists(cpp_filename):
                cpp_files.append(cpp_filename)

        with open('tmp/.devices', 'w') as f:
            f.write(' '.join(config['devices']))
            f.write(' ' + ' '.join(cpp_files))

    elif cmd == '--requirements':
        install_requirements(config['requirements'])

    elif cmd == '--cross-compile':
        with open('tmp/.cross-compile', 'w') as f:
            f.write(config['cross-compile'])

    elif cmd == '--arch-flags':
        with open('tmp/.arch-flags', 'w') as f:
            f.write('"-' + ' -'.join(config['arch_flags']) + '"')

    elif cmd == '--defines':
        with open('tmp/.defines', 'w') as f:
            f.write('"-D' + ' -D'.join(config['defines']) + '"')

    else:
        raise ValueError('Unknown command')

if __name__ == "__main__":
        main(sys.argv[1:])
