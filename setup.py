#!/bin/env python3
import os
import subprocess
import sys
from distutils.spawn import find_executable

from setuptools import setup
from setuptools.command.build_py import build_py
from setuptools.command.develop import develop


def find_protoc():
    if "PROTOC" in os.environ and os.path.exists(os.environ["PROTOC"]):
        protoc = os.environ["PROTOC"]
    else:
        protoc = find_executable("protoc")

    if protoc is None:
        sys.stderr.write(
            "protoc not found. Is protobuf-compiler installed? \n"
            "Alternatively, you can point the PROTOC environment variable at a local version (current: {}).".format(
                os.environ.get("PROTOC", "Not set")
            )
        )
        sys.exit(1)

    return protoc


def make_proto(command):
    proto_dir = command.get_package_dir("metricq_proto")
    print("[protobuf] {}".format(proto_dir))
    for proto_file in filter(lambda x: x.endswith(".proto"), os.listdir(proto_dir)):
        source = os.path.join(proto_dir, proto_file)
        out_file = source.replace(".proto", "_pb2.py")

        if not os.path.exists(out_file) or os.path.getmtime(source) > os.path.getmtime(
                out_file
        ):
            out_dir = command.get_package_dir("metricq")
            sys.stderr.write("[protobuf] {} -> {}\n".format(source, out_dir))
            subprocess.check_call(
                [
                    find_protoc(),
                    "--proto_path=" + proto_dir,
                    "--python_out=" + out_dir,
                    os.path.join(proto_dir, proto_file),
                ]
            )


class ProtoBuildPy(build_py):
    def run(self):
        make_proto(self)
        super().run()


class ProtoDevelop(develop):
    def run(self):
        self.run_command("build_py")
        super().run()


setup(
    name="metricq",
    version="1.1.3",
    author="TU Dresden",
    description="A highly-scalable, distributed metric data processing framework based on RabbitMQ",
    url="https://github.com/metricq/metricq",
    classifiers=[
        "License :: OSI Approved :: BSD License",
        "Programming Language :: Python :: 3",
    ],
    python_requires=">=3.6",
    packages=["metricq", "metricq_proto"],
    scripts=[],
    install_requires=[
        "aio-pika~=6.0,>=6.4.0",
        "aiormq~=3.0",  # TODO: remove once aio-pika reexports ChannelInvalidStateError
        "protobuf>=3",
        "yarl",
    ],
    extras_require={
        "examples": ["aiomonitor", "click", "click-log", "click-completion"]
    },
    cmdclass={"build_py": ProtoBuildPy, "develop": ProtoDevelop},
    package_dir={"": "python", "metricq_proto": "src"},
    test_suite="examples",
)
