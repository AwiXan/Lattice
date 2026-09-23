#!/usr/bin/env python3
"""Runs the editor's self-test on a throwaway project and says how it went.

The editor checks itself when started with LATTICE_SELFTEST set (see
editor/editor_self_test.h): it splits, closes, opens and reopens things the way
a user would, prints a line for every check, counts every error printed, and
quits. What it cannot see from inside is what is left over once it has quit,
so this reads its output for leaked objects as well.

    python misc/scripts/lattice_selftest.py
    python misc/scripts/lattice_selftest.py --editor bin/godot.windows.editor.x86_64.console.exe --keep

Exits 0 when everything passed and nothing was left over.
"""

import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile

PROJECT_FILES = {
    "project.godot": """config_version=5

[application]

config/name="Lattice self-test"
run/main_scene="res://scene_a.tscn"
""",
    "scene_a.tscn": """[gd_scene format=3]

[node name="SceneA" type="Node3D"]

[node name="Mesh" type="MeshInstance3D" parent="."]
""",
    "scene_b.tscn": """[gd_scene format=3]

[node name="SceneB" type="Node2D"]
""",
    "probe.gd": """extends Node


func _ready() -> void:
\tpass
""",
    "probe.gdshader": """shader_type spatial;

void fragment() {
\tALBEDO = vec3(1.0);
}
""",
}


def find_editor():
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    candidates = glob.glob(os.path.join(root, "bin", "godot.*.editor.*"))
    # The console wrapper on Windows, so the output can be read.
    candidates = [c for c in candidates if c.endswith(".console.exe") or not c.endswith(".exe")]
    candidates = [c for c in candidates if not c.endswith((".pdb", ".lib", ".exp", ".ilk"))]
    if not candidates:
        return None
    return max(candidates, key=os.path.getmtime)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--editor", help="editor binary to run (default: the newest one in bin/)")
    parser.add_argument("--keep", action="store_true", help="keep the project and the full output")
    parser.add_argument("--timeout", type=int, default=600, help="seconds before giving up")
    args = parser.parse_args()

    editor = args.editor or find_editor()
    if not editor or not os.path.exists(editor):
        print("No editor binary found; build one or pass --editor.")
        return 2

    project = tempfile.mkdtemp(prefix="lattice-selftest-")
    for name, text in PROJECT_FILES.items():
        with open(os.path.join(project, name), "w", encoding="utf-8", newline="\n") as f:
            f.write(text)

    env = dict(os.environ, LATTICE_SELFTEST="1")
    command = [editor, "--path", project, "--editor", "--headless", "--verbose"]
    print("Running", " ".join(command))
    try:
        result = subprocess.run(
            command,
            env=env,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=args.timeout,
        )
    except subprocess.TimeoutExpired:
        print("The editor did not finish within %d seconds." % args.timeout)
        return 1

    output = result.stdout + "\n" + result.stderr
    lines = output.splitlines()
    checks = [line for line in lines if line.startswith(("SELFTEST PASS", "SELFTEST FAIL"))]
    failures = [line for line in lines if line.startswith("SELFTEST FAIL")]
    done = [line for line in lines if line.startswith("SELFTEST DONE")]
    errors = [line for line in lines if line.lstrip().startswith("ERROR:")]
    leaks = [line for line in lines if "Leaked instance" in line or "resources still in use" in line]

    for line in checks:
        print(line)
    for line in errors:
        print(line)
    for line in leaks:
        print(line)

    ok = result.returncode == 0 and done and not failures and not errors and not leaks
    print(
        "%s: exit %d, %d checks, %d failed, %d errors, %d leaks%s"
        % (
            "OK" if ok else "FAILED",
            result.returncode,
            len(checks),
            len(failures),
            len(errors),
            len(leaks),
            "" if done else ", never finished",
        )
    )

    if args.keep:
        with open(os.path.join(project, "selftest_output.txt"), "w", encoding="utf-8") as f:
            f.write(output)
        print("Project and output kept in", project)
    else:
        shutil.rmtree(project, ignore_errors=True)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
