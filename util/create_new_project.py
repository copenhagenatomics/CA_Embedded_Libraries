#!/usr/bin/env python3

"""
Create a new project from the Template project (both STM32/ and unit_testing/).

Interactively asks for an author name and a project name, then copies
STM32/Template -> STM32/<name> and unit_testing/Template -> unit_testing/<name>,
replacing every occurrence of "Template"/"template"/"TEMPLATE" (in file contents
and file/directory names) with the equivalent casing of the new project name, and
filling in the "AUTHOR"/"DATE" placeholders found in file headers.

The project name may be prefixed with sub-folders (e.g. "folder1/BoardName"), in which
case the project is created in STM32/folder1/BoardName (and unit_testing/folder1/BoardName),
creating the sub-folders if needed and adjusting the relative paths in the copied files.
"""

import os
import re
import subprocess
import sys
from datetime import date

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LIB_ROOT = os.path.dirname(SCRIPT_DIR)      # CA_Embedded_Libraries (this script's own repo)
REPO_ROOT = os.path.dirname(LIB_ROOT)       # Parent repo, which holds CA_Embedded_Libraries
TEMPLATE_NAME = "Template"

# Each entry: destination dir name (created under REPO_ROOT), the git repo the Template
# source lives in, and the Template source path relative to that repo's root.
# Both STM32/Template and unit_testing/Template now live in the CA_Embedded_Libraries
# submodule, each one level deeper under its MCU package folder.
PROJECT_DIRS = [
    ("STM32", LIB_ROOT, "STM32/Template/STM32F401CCU6"),
    ("unit_testing", LIB_ROOT, "unit_testing/Template/STM32F401CCU6"),
]

def lower_first(s):
    """
    Sets the first letter of the project as lower letter
    """
    return s[0].lower() + s[1:]

def to_upper_snake(name):
    """
    PascalCase -> UPPER_SNAKE_CASE by splitting before each internal capital letter,
    e.g. "TestBoard" -> "TEST_BOARD"
    """
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).upper()

def tracked_files(repo_root, rel_dir):
    """
    Paths (relative to rel_dir) of files git tracks under rel_dir
    """
    result = subprocess.run(["git", "ls-files", rel_dir], cwd=repo_root,
                            capture_output=True, text=True, check=True)
    prefix = rel_dir + "/"
    return [line[len(prefix):] for line in result.stdout.splitlines() if line]

def ask_author():
    """
    Get the author name from the user
    """
    while True:
        author = input("Author name: ").strip()
        if author:
            return author
        print("Author name cannot be empty.")

def ask_project_path():
    """
    Get the project path from the user, e.g. "BoardName" or "folder1/BoardName"
    """
    while True:
        path = input("Project name: ").strip().replace("\\", "/").strip("/")
        parts = path.split("/")
        name = parts[-1]
        if not path:
            print("Project name cannot be empty.")
        elif any(c.isspace() for c in path):
            print("Project name must not contain spaces.")
        elif any(p in ("", ".", "..") for p in parts):
            print("Project sub-folders must not be empty, '.' or '..'.")
        elif not name[0].isupper():
            print("Project name must start with a capital letter.")
        elif any(os.path.exists(os.path.join(REPO_ROOT, dst, path)) for dst, _, _ in PROJECT_DIRS):
            print(f'A project named "{path}" already exists.')
        else:
            return path

def expand_name(text, name):
    """
    Replace the three Template casings with the equivalent casing of `name`
    """
    text = text.replace(TEMPLATE_NAME, name)                              # Template -> ProjectName
    text = text.replace(lower_first(TEMPLATE_NAME), lower_first(name))    # template -> projectName
    text = text.replace(TEMPLATE_NAME.upper(), to_upper_snake(name))      # TEMPLATE -> PROJECT_NAME
    return text

def adjust_paths(text, subdir):
    """
    The Template's relative paths assume the project sits directly in STM32/ (or
    unit_testing/). Fix them up for a project placed in sub-folder(s) `subdir` of those
    """
    if not subdir:
        return text
    ups = "../" * (subdir.count("/") + 1)
    text = text.replace("../../", "../../" + ups)                            # -> repo root
    text = text.replace("/../unitTests.py", "/../" + ups + "unitTests.py")  # -> unit_testing/
    text = re.sub(r"/" + TEMPLATE_NAME + r'(?=[/"])',                       # -> other project
                  lambda _: "/" + subdir + "/" + TEMPLATE_NAME, text)
    text = text.replace("-b ${workspaceFolderBasename}",                    # dfu.py board path
                        "-b " + subdir + "/${workspaceFolderBasename}")
    return text

def fill_header_fields(text, author, today):
    """
    Replace AUTHOR/DATA placeholders in file headers
    """
    text = re.sub(r"\bAUTHOR\b", lambda _: author, text)
    text = re.sub(r"\bDATE\b", lambda _: today, text)
    return text

def create_project(dst_dir, src_root, src_rel, path, author, today):
    """
    Create a new project based on the template project
    """
    subdir, _, name = path.rpartition("/")
    src_path = os.path.join(src_root, src_rel)
    dst_root = os.path.join(REPO_ROOT, dst_dir, *path.split("/"))
    created = []
    for rel_path in tracked_files(src_root, src_rel):
        dst_path = os.path.join(dst_root, expand_name(rel_path, name))
        os.makedirs(os.path.dirname(dst_path), exist_ok=True)

        with open(os.path.join(src_path, rel_path), "r", encoding="utf-8") as f:
            content = f.read()
        content = adjust_paths(content, subdir)
        content = expand_name(content, name)
        content = fill_header_fields(content, author, today)

        with open(dst_path, "w", encoding="utf-8") as f:
            f.write(content)
        created.append(os.path.relpath(dst_path, REPO_ROOT))
    return created

def main():
    """
    Entry point of the script
    """
    for _, src_root, src_rel in PROJECT_DIRS:
        if not os.path.isdir(os.path.join(src_root, src_rel)):
            sys.exit(f"error: {os.path.join(src_root, src_rel)} not found")

    author = ask_author()
    path = ask_project_path()
    today = date.today().strftime("%d/%m/%Y")

    created = []
    for dst_dir, src_root, src_rel in PROJECT_DIRS:
        created += create_project(dst_dir, src_root, src_rel, path, author, today)

    print(f"\nCreated {len(created)} files for project '{path}':")
    for path in created:
        print(f"  {path}")

    print("New project successfully created")
    print("Next steps:")
    print("  Create a new OTP for your project (systeminfo.h/.c files)")
    print("  Open the .ioc file with CubeMX to customize the board")

if __name__ == "__main__":
    main()
