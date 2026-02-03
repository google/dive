# #!/usr/bin/env python3

# Copyright 2026 Google LLC
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

"""Combines //third_party/gfxreconstruct/.gitmodules with //.gitmodules.

This is required for a successful subtree pull of GFXR.

Limitations:

- If GFXR deletes a submodule, this won't detect that. This will only add
  new submodules or update existing ones.
- Does not handle git config multi-value variables
- Does not preserve whitespace at the end of git config values
"""

import argparse
import dataclasses
import pathlib
import subprocess
import shutil
import sys


@dataclasses.dataclass
class ConfigName:
    """A Python representation of a git config variable name.

    https://git-scm.com/docs/git-config#_syntax
    """

    section: str
    subsection: str | None
    key: str

    def __str__(self):
        if self.subsection is None:
            return f"{self.section}.{self.key}"

        return f"{self.section}.{self.subsection}.{self.key}"


@dataclasses.dataclass
class ConfigVariable:
    """A Python representation of a git config variable.

    https://git-scm.com/docs/git-config#_syntax
    """

    name: ConfigName
    value: str

    def __str__(self):
        return f"{self.name}={self.value}"


class MissingSectionError(Exception):
    """The git config name couldn't be parsed since it's missing a section."""

    pass


class InvalidKeyError(Exception):
    """The git config name couldn't be parsed since the key contains an
    invalid character.
    """

    pass


def parse_name(name: str) -> ConfigName:
    """Parses a git config name for its component parts.

    Args:
        name: A name produced by `git config list --name-only`

    Returns:
        The name decomposed into its component parts for easier processing.

    Raises:
        MissingSectionError: If the name doesn't have a section
        InvalidKeyError: If the key part of the name contains an invalid
          character
    """
    parts = name.split(".")
    parts_len = len(parts)
    if parts_len < 2:
        raise MissingSectionError(name)

    section = parts[0]
    key = parts[-1]
    if "=" in key:
        raise InvalidKeyError(key)

    if parts_len > 2:
        subsection = ".".join(parts[1:-1])
    else:
        subsection = None

    return ConfigName(section=section, subsection=subsection, key=key)


def git_config_list_name_only(
    git: pathlib.Path, config_file: pathlib.Path
) -> list[ConfigName]:
    """Runs `git config list --name-only` and parses the output.

    Args:
        git: Path to the git executable
        config_file: Path to the config file to list

    Returns:
        All parsed names from `git config list --name-only`

    Raises:
        subprocess.CompletedProcessError: If git returns a non-zero exit code
        MissingSectionError: Couldn't parse the file since a section is
          missing from a name
        InvalidKeyError: Couldn't prase the file since a name contains a key
          with an invalid character
    """
    process = subprocess.run(
        [str(git), "config", "list", "-f", str(config_file), "--name-only"],
        text=True,
        capture_output=True,
    )
    process.check_returncode()
    return [parse_name(name) for name in process.stdout.splitlines()]


def git_config_get(
    git: pathlib.Path, config_file: pathlib.Path, name: ConfigName
) -> str:
    """Runs `git config get` and returns the stdout.

    Args:
        git: Path to the git executable
        config_file: Path to the config file to read
        name: The name of the value to get

    Returns:
        The value of the name in the config.

    Raises:
        subprocess.CompletedProcessError: If git returns a non-zero exit code
    """
    process = subprocess.run(
        [
            str(git),
            "config",
            "get",
            "-f",
            str(config_file),
            str(name),
        ],
        text=True,
        capture_output=True,
    )
    process.check_returncode()
    # git prints a trailing whitespace which we want to remove. Unfortunately,
    # this will also remove any whitespace that is intentionally in the config
    # file. This is not a requirement so I kept the simplest solution.
    return process.stdout.rstrip()


def git_config_set(
    git: pathlib.Path, config_file: pathlib.Path, variable: ConfigVariable
):
    """Runs `git config set` to modify a variable.

    Args:
        git: Path to the git executable
        config_file: Path to the config file to modify
        variable: The name to modify and the value to set it to.

    Raises:
        subprocess.CompletedProcessError: If git returns a non-zero exit code
    """
    subprocess.run(
        [
            str(git),
            "config",
            "set",
            "-f",
            str(config_file),
            str(variable.name),
            str(variable.value),
        ],
        text=True,
        capture_output=True,
    ).check_returncode()


def list_variables(
    git: pathlib.Path, config_file: pathlib.Path
) -> list[ConfigVariable]:
    """Lists all variables in a git config file.

    Args:
        git: Path to the git executable
        config_file: Path to the config file to list

    Returns:
        All parsed names from `git config list --name-only`

    Raises:
        subprocess.CompletedProcessError: If git returns a non-zero exit code
        MissingSectionError: Couldn't parse the file since a name is missing
          the section
        InvalidKeyError: Couldn't prase the file since a name contains a key
          with an invalid character
    """
    return [
        ConfigVariable(name, git_config_get(git, config_file, name))
        for name in git_config_list_name_only(git, config_file)
    ]


def rebase_config_variable(variable: ConfigVariable, prefix: str) -> ConfigVariable:
    """Relocates a submodule into a different directory.

    Args:
        variable: The config to modify
        prefix: The value prepended in order to rebase

    Returns:
        A new variable with the subsection name and path values prepended with
        the prefix
    """
    new_name = ConfigName(
        section=variable.name.section,
        subsection=f"{prefix}/{variable.name.subsection}",
        key=variable.name.key,
    )

    if variable.name.key == "path":
        new_value = f"{prefix}/{variable.value}"
    else:
        new_value = variable.value

    return ConfigVariable(new_name, new_value)


def main(args: argparse.Namespace):
    try:
        variables = list_variables(args.git_path, args.override_gitmodules)
    except subprocess.CalledProcessError as error:
        print(f"subprocess.CalledProcessError: {error}")
        if error.stdout:
            print(f"stdout: {error.stdout}")
        if error.stderr:
            print(f"stderr: {error.stderr}")
        sys.exit(1)

    prefix = args.override_gitmodules.relative_to(args.base_gitmodules.parent).parent

    for variable in variables:
        if variable.name.subsection == "external/OpenXR-Docs":
            print("Skipping 'external/OpenXR-Docs' since it's not required")
            continue

        git_config_set(
            args.git_path,
            args.base_gitmodules,
            rebase_config_variable(variable, prefix),
        )


def parse_args() -> argparse.Namespace:
    repo_base_path = pathlib.Path(__file__).parent / ".."
    parser = argparse.ArgumentParser(
        description="Combines additions from an override .gitmodules to a base .gitmodules."
    )
    parser.add_argument(
        "--git_path",
        type=pathlib.Path,
        default=shutil.which("git"),
        help="Location of the git executable to run",
    )
    parser.add_argument(
        "--base_gitmodules",
        type=pathlib.Path,
        default=repo_base_path / ".gitmodules",
        help="The .gitmodules files to update",
    )
    parser.add_argument(
        "--override_gitmodules",
        type=pathlib.Path,
        default=repo_base_path / "third_party" / "gfxreconstruct" / ".gitmodules",
        help="The .gitmodules file with additions that we want to incorporate",
    )
    return parser.parse_args()


if __name__ == "__main__":
    main(parse_args())
