# Passes OTA_PASSWORD from main/secrets.h to espota, so the password lives in one place
import re
import sys
from os.path import join

from SCons.Script import COMMAND_LINE_TARGETS

Import("env")


def read_password():
    try:
        with open(join(env.subst("$PROJECT_SRC_DIR"), "secrets.h"), encoding="utf-8") as f:
            match = re.search(r'^\s*const char\*\s*OTA_PASSWORD\s*=\s*"([^"]*)"', f.read(), re.MULTILINE)
            return match.group(1) if match else ""
    except FileNotFoundError:
        return ""


# building this environment needs no password, only uploading does
if "upload" in COMMAND_LINE_TARGETS:
    password = read_password()
    if not password:
        sys.stderr.write("Error: set OTA_PASSWORD in main/secrets.h and flash the board once over USB\n")
        env.Exit(1)

    # the password ends up on a shell command line
    if not re.fullmatch(r"[A-Za-z0-9._-]+", password):
        sys.stderr.write('Error: OTA_PASSWORD may only use letters, digits, ".", "-" and "_"\n')
        env.Exit(1)

    env.Append(UPLOADERFLAGS=["--auth=" + password])
