# PlatformIO pre-script: FW_VERSION from $FW_VERSION (CI) or `git describe`.
import os, subprocess
Import("env")
v = os.environ.get("FW_VERSION")
if not v:
    try:
        v = subprocess.check_output(["git", "describe", "--tags", "--always", "--dirty"], text=True).strip()
    except Exception:
        v = "dev"
env.Append(CPPDEFINES=[("FW_VERSION", '\\"%s\\"' % v)])
