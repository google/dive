$ErrorActionPreference = "Stop"

if (Test-Path env:PYTHON_EXECUTABLE) {
    $python = $env:PYTHON_EXECUTABLE
}
else {
    $python = (Get-Command python).source
}
$python_ver = & $python --version

$dive_src = git rev-parse --show-toplevel

$env = Join-Path $dive_src ".env"

if (Test-Path env:JUPYTER_KERNEL_NAME) {
    $ker_name = $env:JUPYTER_KERNEL_NAME
}
else {
    $ker_name = "Dive"
}

"Python: $python ($python_ver)"
"Dive Src: $dive_src"
"Env: $env"

if (Test-Path $env) {
    "$env directory exists."
}
else {
    # Create virtual environment
    & $python -m venv $env
}

# Activate virtual environment
& (Join-Path $env "Scripts/Activate.ps1")

if (-not (Get-Command python).source.StartsWith($env)) {
    $venv_python = Get-Command python
    "python resolved to $venv_python after activating virtual environment."
    "Expected python to resolve to a file in $env. Exiting."
    exit -1
}

# Register this virtual environment as a Jupyter kernel
python -m pip install --upgrade pip
python -m pip install ipykernel
python -m ipykernel install --user --name=$ker_name

# Install Dive Python package in virtual environment
Set-Location (Join-Path $dive_src "pydive")
python setup.py bdist_wheel
python -m pip install (Get-Item dist/dive-*.whl).FullName --upgrade