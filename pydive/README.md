# Setup Python and Jupyter for Dive

## 1. Set Up Python Environment
### 1.1 Install Python >= 3.7 (x64)
- Windows: I already had a Python installation from Visual Studio ("C:\Program Files
  (x86)\Microsoft Visual Studio\Shared\Python37_64\python.exe"). Otherwise, you can use the
  **x86-64** installer from
  [python.org/downloads/windows](https://www.python.org/downloads/windows). **NOTE:** the default
  download is for 32-bit Python, which wont link with our 64-bit dive library.
- GLinux: Install "python3-dev" package
- Verify that `python` is in your path, and refers to the correct Python version.
  
  On some systems, `python` refers to Python 2.7, and `python3` is version 3.x. In that case, you
  can adjust your path or just set a temporary alias (bash):
    ```
    $ alias python=python3
    ```

### 1.2. Install Node.js

The plotting library used for the visualizations depends on Node.js

- Windows: Install Node.js from https://nodejs.org/ (12.* LTS)
- Linux: `apt-get install nodejs`

### 1.3. Install jupyter 
This assumes the correct `python` executable is in your path.
```
$ python -m pip install jupyterlab
```
This installs a "jupyter" executable into Python script directory. If that directory is not in your
path, installing jupyter should give you a warning, and tell you what you need to add to your path.
On my Windows machine, the path was "<home directory>/AppData/Roaming/Python/Python37/Scripts". On
Linux, this path was "$HOME/.local/bin".

## 2. Build Dive Python bindings (C++ code)
1. Initialize the pybind11 submodule: `git submodule update --init`
2. Add the following to your `cmake config` command:
    ```
    -DDIVE_PYTHON_BINDINGS=On -DPYTHON_EXECUTABLE=/path/to/python
    ```
    E.g.:
    - Linux:
        ```
        $ cmake -GNinja -DPYTHON_EXECUTABLE=/usr/bin/python3 -DDIVE_PYTHON_BINDINGS=On ..
        ```
    - Windows:
        ```
        $ cmake -G "Visual Studio 16 2019" -Ax64 -DDIVE_PYTHON_BINDINGS=On \
        -DPYTHON_EXECUTABLE="C:\Program Files (x86)\Microsoft Visual Studio\Shared\Python37_64\python.exe" ..
        ```
    *Note:* If you run into cmake errors here, try specifying the specifying the Python include
    and/or lib directory:
    ```
    -DPYTHON_INCLUDE_DIR="/path/to/python/include" -DPYTHON_LIBRARY="/path/to/python/libs"
    ```
3. Build Dive as usual. You should see a new folder "pydive" in the build directory

## 3. Setup a Python virtual environment with the Dive Python package

These scripts assume the `python` in your path is the version passed to "-DPYTHON_EXECUTABLE" to
build dive.

- Windows:
    ```
    $ cd dive-src
    $ ./scripts/create-python-env.ps1
    ```
- Linux:
    ```
    $ cd dive-src
    $ ./scripts/create-python-env.sh
    ```

These scripts create a Python [virtual environment](https://docs.python.org/3/tutorial/venv.html),
which acts as an independent Python installation, with it's own set of packages.

The virtual environment is directory is in "dive-src/.env".

The virtual environment is registered with Jupyter as a "Kernel" (i.e. a Python interpreter that
can be selected for evaluating notebooks). The name of the kernel is "Dive".

When you rebuild dive, or update the Python code in "pydive/dive", you can re-run this script to
update the Dive Python package in the virtual environment.

# Jupyter Notebook Quickstart
1. Start jupyter. The directory where you run `jupyter lab` determines the top level directory in
   the notebook picker in the web app, so I run this from the root dive source directory.
   
   *Note:* You don't need activate the virtual environment in the shell you start Jupyter (because
   Jupyter handles the virtual environments itself, as "Kernels").
    ```
    $ cd /path/to/dive-src
    $ jupyter lab
    ```
    This should print out a url, of the form "http://localhost:8888/?token=123abc".

    This should also automatically open this url in a browser window.

2. In the browser window, there should be a file tree on the left. Open the "notebooks" directory
   in the dive source tree, and then open your notebook--try "Demo1.ipynb" first, which includes an
   introduction on how to use a Jupyter notebook.

3. Change the `path` variable to a path to a dive capture on your machine.

4. In the menu at the top, click on "Run > Run All Cells". Most cells should show output (underneath the code). You should not see any scary looking errors in a red background.

# More information about Jupyter notebooks

A Jupyter notebook is a file containing a series of "cells". Cells can contain Python code or markdown. The code cells can be "run"--this executes the code, and shows any output.

Cells all run in the same interpreter, so e.g. a variable can be defined in one cell and referenced
in another cell.

You can run a single cell with <kbd>SHIFT</kbd>+<kbd>ENTER</kbd>. You can also run all cells/subset of cells through the "Run" menu.

You can run them in any order. You can run cells multiple times, and you can modify cells and run them again.

Running a cell does *not* implicitly run the earlier cells--if I try to run a cell that references
a variable defined in an earlier cell that I haven't run yet, the variable will be undefined.

## Ouptut

Output from code cells is shown below the cell when it is run. There are multiple ways to produce output:

- `print()` E.g.
- The return value of the last command in the cell. Some types have fancy formatted output that are automatically applied--e.g. if the last statement in the cell is a `DataFrame`, it is displayed as a formatted table.
- Visualizations/Widgets

## Visualizations

There are several visualization libraries for Python. Currently, I have mostly been using [bokeh](https://docs.bokeh.org/en/latest/index.html). There is a prototype occupancy view visualization, similar to the Dive UI. See the "notebooks/Visualization.ipynb" notebook for examples.

# Jupyter Notebooks and Git

Jupyter notebooks don't work particularly well with Git:
- The .ipynb format is not particularly readable as raw text, making the diffs noisy and
  uninformative.
- The .ipynb files include the output from each cell, so every time you run a cell, the .ipynb
  changes, even if you don't touch the code.

I find that most of the Jupyter notebook flow is experimental, and I usually don't want to track my
changes to notebooks, so I made 2 directories for notebooks in the Dive repo:
- "notebooks": Notebooks that are managed by git
- "local_notebooks": Notebooks that are ignored by git

