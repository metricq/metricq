#!/bin/env python3
import os
import subprocess
import sys

from setuptools import setup
from setuptools.command.build_py import build_py
from setuptools.command.develop import develop
from distutils.spawn import find_executable


def find_protoc():
    if 'PROTOC' in os.environ and os.path.exists(os.environ['PROTOC']):
        protoc = os.environ['PROTOC']
    else:
        protoc = find_executable('protoc')

    if protoc is None:
        sys.stderr.write('protoc not found. Is protobuf-compiler installed? \n'
                         'Alternatively, you can point the PROTOC environment variable at a local version.')
        sys.exit(1)

    return protoc


def make_proto(command):
    proto_dir = command.get_package_dir('dataheap2_proto')
    print('[protobuf] {}\n'.format(proto_dir))
    for proto_file in filter(lambda x: x.endswith('.proto'), os.listdir(proto_dir)):
        source = os.path.join(proto_dir, proto_file)
        out_file = source.replace('.proto', '_pb2.py')

        if not os.path.exists(out_file) or os.path.getmtime(source) > os.path.getmtime(out_file):
            out_dir = command.get_package_dir('dataheap2')
            sys.stderr.write('[protobuf] {} -> {}\n'.format(source, out_dir))
            subprocess.check_call([find_protoc(), '--proto_path=' + proto_dir,
                                   '--python_out=' +
                                   out_dir,
                                   proto_file])


class ProtoBuildPy(build_py):
    def run(self):
        make_proto(self)


class ProtoDevelop(develop):
    def run(self):
        self.run_command('build_py')


setup(name='dataheap2',
      version='0.0',
      author='TU Dresden',
      python_requires=">=3.5",
      packages=['dataheap2', 'dataheap2_proto'],
      scripts=[],
      install_requires=['aio-pika'],
      cmdclass={'build_py': ProtoBuildPy, 'develop': ProtoDevelop},
      package_dir={'': 'python', 'dataheap2_proto': 'src'})
