"""Script to build C extensions."""

import os

from setuptools import Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.develop import develop
from setuptools.errors import CCompilerError, ExecError, PlatformError

extensions = [
    Extension(
        "pubmedparser._readxml",
        sources=["pubmedparser/_readxml.c"],
        include_dirs=["include"],
        extra_compile_args=["-O3", "-fopenmp"],
        extra_link_args=["-fopenmp"],
        libraries=["z", "pubmedparser"],
    ),
]


class ExtBuilder(build_ext):
    def run(self):
        try:
            build_ext.run(self)
        except FileNotFoundError:
            print("Failed to build C extension.")

    def build_extension(self, ext):
        try:
            build_ext.build_extension(self, ext)
        except (
            CCompilerError,
            ExecError,
            PlatformError,
            ValueError,
        ):
            print("Failed to build C extension.")


class CustomDevelop(develop):
    def run(self):
        self.run_command("build_clib")
        super().run()


def build(setup_kwargs):
    c_files = [
        "read_xml_core.c",
        "paths.c",
        "query.c",
        "nodes.c",
        "error.c",
        "yaml_reader.c",
        "read_structure_file.c",
    ]
    setup_kwargs.update(
        {
            "libraries": [
                (
                    "pubmedparser",
                    {
                        "sources": [os.path.join("src", f) for f in c_files],
                        "include_dirs": ["include"],
                        "libraries": ["z"],
                        "cflags": ["-O3", "-fopenmp"],
                    },
                )
            ],
            "ext_modules": extensions,
            "cmdclass": {"develop": CustomDevelop},
        }
    )
