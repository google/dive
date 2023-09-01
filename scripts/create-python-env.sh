#!/bin/bash

# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


if [ -z "$PYTHON_EXECUTABLE" ]
then
    PYTHON_EXECUTABLE=$(which python3)
fi

[[ "$($PYTHON_EXECUTABLE -V)" =~ "Python 3."[789] ]] || (echo "Requires Python 3.7 or higher" && exit -1)

DIVE_SRC=$(git rev-parse --show-toplevel)
ENV="$DIVE_SRC/.env"

if [ -z "$JUPYTER_KERNEL_NAME" ]
then
    JUPYTER_KERNEL_NAME="Dive"
fi

echo "Python: $PYTHON_EXECUTABLE ($($PYTHON_EXECUTABLE -V))"
echo "Dive Src: $DIVE_SRC"
echo "Env: $ENV"

if [ -d $ENV ]
then
    "$ENV directory exists."
else
    "Creating virtual environment in $ENV"
    $PYTHON_EXECUTABLE -m venv $ENV
fi

source $ENV/bin/activate

# Register this virtual environment as a Jupyter kernel
python -m pip install --upgrade pip
python -m pip install ipykernel
python -m ipykernel install --user --name=$JUPYTER_KERNEL_NAME

# Install Dive Python package in virtual environment
pushd "$DIVE_SRC/pydive"
python -m pip install wheel
python setup.py bdist_wheel
python -m pip install dist/dive-*.whl --upgrade
popd