#!/usr/bin/env python3

"""
Create a new project from the Template project (both STM32/ and unit_testing/).

Interactively asks for an author name and a project name, then copies
STM32/Template -> STM32/<name> and unit_testing/Template -> unit_testing/<name>,
replacing every occurrence of "Template"/"template"/"TEMPLATE" (in file contents
and file/directory names) with the equivalent casing of the new project name, and
filling in the "AUTHOR"/"DATE" placeholders found in file headers.
"""

import os
import re
import subprocess
import sys
from datetime import date

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEMPLATE_NAME = "Template"
PROJECT_DIRS = ["STM32", "unit_testing"]

def lower_first(s):
    return s[0].lower() + s[1:]

def to_upper_snake(name):
    # PascalCase -> UPPER_SNAKE_CASE by splitting before each internal capital letter,
    # e.g. "TestBoard" -> "TEST_BOARD"
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).upper()

def tracked_files(rel_dir):
    """Paths (relative to rel_dir) of files git tracks under rel_dir.

    Using git as the source of truth avoids dragging along local, gitignored
    artifacts that may exist in the Template folder (build/, githash.h, ...).
    """
    result = subprocess.run(["git", "ls-files", rel_dir], cwd=REPO_ROOT,
                            capture_output=True, text=True, check=True)
    prefix = rel_dir + "/"
    return [line[len(prefix):] for line in result.stdout.splitlines() if line]

def ask_author():
    while True:
        author = input("Author name: ").strip()
        if author:
            return author
        print("Author name cannot be empty.")

def ask_project_name():
    while True:
        name = input("Project name: ").strip()
        if not name:
            print("Project name cannot be empty.")
        elif any(c.isspace() for c in name) or "/" in name or "\\" in name:
            print("Project name must not contain spaces or path separators.")
        elif not name[0].isupper():
            print("Project name must start with a capital letter.")
        elif any(os.path.exists(os.path.join(REPO_ROOT, d, name)) for d in PROJECT_DIRS):
            print(f'A project named "{name}" already exists.')
        else:
            return name

def expand_name(text, name):
    """Replace the three Template casings with the equivalent casing of `name`."""
    text = text.replace(TEMPLATE_NAME, name)                              # Template -> ProjectName
    text = text.replace(lower_first(TEMPLATE_NAME), lower_first(name))    # template -> projectName
    text = text.replace(TEMPLATE_NAME.upper(), to_upper_snake(name))      # TEMPLATE -> PROJECT_NAME
    return text

def fill_header_fields(text, author, today):
    # AUTHOR/DATE are standalone placeholders in file headers (e.g. "@author  AUTHOR"), so they
    # are matched as whole words -- a plain substring replace would also corrupt unrelated
    # identifiers such as GIT_DATE or TIM_TRGO_UPDATE.
    text = re.sub(r"\bAUTHOR\b", lambda _: author, text)
    text = re.sub(r"\bDATE\b", lambda _: today, text)
    return text

def create_project(rel_dir, name, author, today):
    src_root = os.path.join(REPO_ROOT, rel_dir, TEMPLATE_NAME)
    dst_root = os.path.join(REPO_ROOT, rel_dir, name)
    created = []
    for rel_path in tracked_files(f"{rel_dir}/{TEMPLATE_NAME}"):
        dst_path = os.path.join(dst_root, expand_name(rel_path, name))
        os.makedirs(os.path.dirname(dst_path), exist_ok=True)

        with open(os.path.join(src_root, rel_path), "r", encoding="utf-8") as f:
            content = f.read()
        content = expand_name(content, name)
        content = fill_header_fields(content, author, today)

        with open(dst_path, "w", encoding="utf-8") as f:
            f.write(content)
        created.append(os.path.relpath(dst_path, REPO_ROOT))
    return created

def main():
    for rel_dir in PROJECT_DIRS:
        if not os.path.isdir(os.path.join(REPO_ROOT, rel_dir, TEMPLATE_NAME)):
            sys.exit(f"error: {rel_dir}/{TEMPLATE_NAME} not found")

    author = ask_author()
    name = ask_project_name()
    today = date.today().strftime("%d/%m/%Y")

    created = []
    for rel_dir in PROJECT_DIRS:
        created += create_project(rel_dir, name, author, today)

    print(f"\nCreated {len(created)} files for project '{name}':")
    for path in created:
        print(f"  {path}")

    print("New project successfully created")
    print("Next steps:")
    print("  Create a new OTP for your project (systeminfo.h/.c files)")
    print("  Open the .ioc file with CubeMX to customize the board")

if __name__ == "__main__":
    main()
