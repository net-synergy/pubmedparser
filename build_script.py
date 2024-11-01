"""Script to build C extensions."""

import os

from setuptools import Extension
from setuptools.command.develop import develop

extensions = [
    Extension(
        "pubmedparser._readxml",
        sources=["pubmedparser/_readxml.c"],
        include_dirs=["include"],
        libraries=["pubmedparser", "z", "pthread"],
    ),
]


class CustomDevelop(develop):
    def run(self):
        self.run_command("build_clib")
        super().run()


def build(setup_kwargs):
    print("Running build")
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
                        "libraries": ["z", "pthread"],
                    },
                )
            ],
            "ext_modules": extensions,
            "cmdclass": {"develop": CustomDevelop},
        }
    )
