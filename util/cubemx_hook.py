#!/usr/bin/env python3

"""
CubeMX hook to use library firmware files

CubeMX only ever manages files under the project directory itself; it can't be told that
some sources live in ../../CA_Embedded_Libraries. So the Makefile it writes is patched
around each generation:
  - pre:  ../../CA_Embedded_Libraries/... -> absolute path, matching CubeMX's own form for
          paths outside the project. CubeMX then leaves these entries untouched (it doesn't
          recognise them as one of its own IPs) while it regenerates everything else.
  - post: absolute path -> relative again, and USB_DEVICE (which CubeMX always regenerates
          locally) is deleted and repointed at the shared library copy.
The project may sit at any depth below STM32/ (e.g. STM32/folder1/BoardName), so the number
of "../" leading to the repo root is taken from the wrapper path CubeMX passes in.
Before code gen: python cubemx_hook.py pre  "<wrapper-path>"
After:           python cubemx_hook.py post "<wrapper-path>"
"""

import os
import re
import shutil
import sys

LIB = "CA_Embedded_Libraries"
SRC_EXT = ".c"

# USB_DEVICE is generated fresh by CubeMX on every run, so its local copy is dropped in favour
# of the shared, already-customised library copy (see _use_lib_usb_device).
USB_DEVICE = "USB_DEVICE"
USB_DEVICE_LIB_SUBDIR = "/STM32/FirmwarePackages/STM32F401CCUx/USB_DEVICE"
USB_DEVICE_SOURCES = ["App/usb_device.c", "App/usbd_cdc_if.c",
                      "App/usbd_desc.c", "Target/usbd_conf.c"]
USB_DEVICE_INC_DIRS = ["App", "Target"]

def _find_block(text, varname):
    """
    Match a 'VARNAME = \\\n entry \\\n ... \\\n entry' block, up to its trailing blank line.
    Tolerates CRLF, since CubeMX writes the Makefile with Windows line endings
    """
    return re.search(r"(?ms)^(" + re.escape(varname) +
                     r"\s*=[^\r\n]*\r?\n)(.*?)(\r?\n[ \t]*\r?\n)", text)

def _filter_block(text, varname, drop):
    """
    Remove entries matching drop() from a 'VAR = \\\n entry \\\n ... \\\n entry' block
    """
    m = _find_block(text, varname)
    if not m:
        return text
    header, body, sep = m.groups()
    entries = [e for e in (l.strip().rstrip("\\").strip()
                           for l in body.splitlines()) if e and not drop(e)]
    new_body = " \\\n".join(entries)  # sep already supplies the newline after the last entry

    return text[:m.start()] + header + new_body + sep + text[m.end():]

def _ensure_entries(text, varname, wanted):
    """
    Add any entries from wanted[] that aren't already in the 'VAR = ...' block
    """
    m = _find_block(text, varname)
    if not m:
        return text
    header, body, sep = m.groups()
    entries = [l.strip().rstrip("\\").strip() for l in body.splitlines() if l.strip()]
    missing = [e for e in wanted if e not in entries]
    if not missing:
        return text
    entries += missing
    new_body = " \\\n".join(entries)  # sep already supplies the newline after the last entry

    return text[:m.start()] + header + new_body + sep + text[m.end():]

def _fix_ioc_firmware_package_path(proj):
    """
    CubeMX on Windows writes ProjectManager.CustomerFirmwarePackage with backslashes, each
    escaped as a doubled backslash in the .ioc's Java-properties format (e.g.
    ..\\\\..\\\\CA_Embedded_Libraries\\\\...); normalize that one line back to the forward-slash
    form used elsewhere in the repo (and expected by CubeMX on Linux)
    """
    for name in os.listdir(proj):
        if not name.endswith(".ioc"):
            continue
        ioc = os.path.join(proj, name)
        text = open(ioc, newline="", encoding="utf-8").read()
        new_text = re.sub(
            r"(?m)^(ProjectManager\.CustomerFirmwarePackage=)(.*)$",
            lambda m: m.group(1) + re.sub(r"\\+", "/", m.group(2)),
            text,
        )
        if new_text != text:
            open(ioc, "w", newline="", encoding="utf-8").write(new_text)

def _use_lib_usb_device(text, proj, rel_lib):
    """
    Drop the local USB_DEVICE/ entries CubeMX just wrote, point the Makefile at the shared
    library copy instead, and delete the local folder CubeMX regenerated this run
    """
    usb_dir = os.path.join(proj, USB_DEVICE)
    usb_lib_dir = rel_lib + USB_DEVICE_LIB_SUBDIR
    if not os.path.isdir(usb_dir):
        return text  # nothing generated this run (e.g. USB_DEVICE not enabled)
    text = _filter_block(text, "C_SOURCES", lambda e: e.startswith(USB_DEVICE + "/"))
    text = _filter_block(text, "C_INCLUDES", lambda e: e.startswith("-I" + USB_DEVICE + "/"))
    text = _ensure_entries(text, "C_SOURCES", [usb_lib_dir + "/" +
                                               f for f in USB_DEVICE_SOURCES])
    text = _ensure_entries(text, "C_INCLUDES", ["-I" + usb_lib_dir + "/" +
                                                d for d in USB_DEVICE_INC_DIRS])
    shutil.rmtree(usb_dir)
    return text

def main():
    """
    Entry point of the script
    """
    mode = sys.argv[1]
    invoked = sys.argv[2]

    # CubeMX calls this script with the wrapper's own configured path (e.g.
    # ".../STM32/Template/../../CA_Embedded_Libraries/util/pre_gen_cubemx.bat"), literal ".."
    # and all. Splitting on the first "/../" recovers the project directory without needing
    # to resolve it, and the run of ".." that follows gives the project's depth in the repo.

    m = re.match(r"(.*?)((?:[\\/]\.\.)+)[\\/]", invoked)
    proj = m.group(1)
    ups = m.group(2).count("..")
    rel_lib = "../" * ups + LIB
    mk = os.path.join(proj, "Makefile")

    if not os.path.isfile(mk):
        print("cubemx_hook: no Makefile at", mk, file=sys.stderr)
        return
    text = open(mk, newline="", encoding="utf-8").read()

    # Relative to absolute paths
    if mode == "pre":
        target = os.path.abspath(os.path.join(proj, *[".."] * ups, LIB)).replace("\\", "/")
        text = re.sub(r"(?m)(^|[ =])(?:-I)?" + re.escape(rel_lib) + r"([^ ]*)",
                      lambda m: m.group(1) + target + m.group(2), text)
    # Absolute to relative paths
    else:
        def repl(m):
            b, tail = m.group(1), m.group(2)
            rel = rel_lib + tail
            if tail.endswith(SRC_EXT):
                return b + rel                            # source file: no -I
            return ("" if b == "-I" else b) + "-I" + rel  # include dir: ensure one -I

        text = re.sub(r"(?m)(^|[ =]|-I)(?:[A-Za-z]:)?[\\/][^ ]*?[\\/]" +
                      re.escape(LIB) + r"([^ ]*)", repl, text)

        # Put each library path on its own continued line
        text = re.sub(r" (-I)?(" + re.escape(rel_lib) + r")",
                      lambda m: " \\\n" + (m.group(1) or "") + m.group(2), text)
        text = _use_lib_usb_device(text, proj, rel_lib)
        _fix_ioc_firmware_package_path(proj)

    open(mk, "w", newline="", encoding="utf-8").write(text)

if __name__ == "__main__":
    main()
