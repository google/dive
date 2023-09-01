"""
 Copyright 2023 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """

import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion
from distutils.file_util import copy_file


class PrebuiltExtension(Extension):

    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CopyPrebuilt(build_ext):

    def run(self):
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        ext_dir, ext_filename = os.path.split(self.get_ext_fullpath(ext.name))
        ext_dir = os.path.abspath(ext_dir)

        cfg = os.environ.get('DIVE_CONFIG', 'Release')
        sourcedir = ext.sourcedir
        if platform.system() == "Windows":
            sourcedir = os.path.join(sourcedir, cfg)
        copy_file(os.path.join(sourcedir, ext_filename),
                  os.path.join(ext_dir, ext_filename),
                  verbose=self.verbose,
                  dry_run=self.dry_run)


setup(
    name='dive',
    version='0.0.1',
    author='',
    author_email='',
    description='Python bindings for the Dive GPU profiler',
    long_description='',
    ext_modules=[PrebuiltExtension('_dive', sourcedir='../build/pydive')],
    cmdclass=dict(build_ext=CopyPrebuilt),
    zip_safe=False,
    packages=find_packages(),
    install_requires=['wheel', 'numpy', 'pandas', 'bokeh', 'numba', 'ipywidgets'],
)