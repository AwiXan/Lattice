#!/usr/bin/env python3
"""Takes a picture of the editor with a 3D scene open, for checking how it looks.

The editor does it itself when started with LATTICE_SHOT (see
editor/editor_self_test.h): it opens the scene, selects a node, waits for the
view to settle, saves the window and quits. This needs a real window - it
opens one for a few seconds - because headless draws nothing.

    python misc/scripts/lattice_screenshot.py shot.png
    python misc/scripts/lattice_screenshot.py shot.png --scene 2d
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

from lattice_selftest import find_editor

SCENE_3D = """[gd_scene format=3]

[sub_resource type="BoxMesh" id="box"]

[sub_resource type="PlaneMesh" id="floor"]
size = Vector2(12, 12)

[node name="Level" type="Node3D"]

[node name="Crate" type="MeshInstance3D" parent="."]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.5, 0)
mesh = SubResource("box")

[node name="Pillar" type="MeshInstance3D" parent="."]
transform = Transform3D(1, 0, 0, 0, 3, 0, 0, 0, 1, 3, 1.5, -2)
mesh = SubResource("box")

[node name="Floor" type="MeshInstance3D" parent="."]
mesh = SubResource("floor")
"""

SCENE_2D = """[gd_scene format=3]

[node name="Hud" type="Control"]
layout_mode = 3
anchors_preset = 15
anchor_right = 1.0
anchor_bottom = 1.0

[node name="Card" type="Panel" parent="."]
layout_mode = 1
anchors_preset = 8
anchor_left = 0.5
anchor_top = 0.5
anchor_right = 0.5
anchor_bottom = 0.5
offset_left = -160.0
offset_top = -100.0
offset_right = 160.0
offset_bottom = 100.0
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("output", help="where to save the picture (.png)")
    parser.add_argument("--scene", choices=["3d", "2d"], default="3d")
    parser.add_argument("--crop", help="x,y,w,h of the window to keep, in pixels")
    parser.add_argument("--editor", help="editor binary to run (default: the newest one in bin/)")
    args = parser.parse_args()

    editor = args.editor or find_editor()
    if not editor:
        print("No editor binary found; build one or pass --editor.")
        return 2

    project = tempfile.mkdtemp(prefix="lattice-shot-")
    with open(os.path.join(project, "project.godot"), "w", encoding="utf-8", newline="\n") as f:
        f.write('config_version=5\n\n[application]\n\nconfig/name="Lattice screenshot"\n')
    scene = "scene_3d.tscn" if args.scene == "3d" else "scene_2d.tscn"
    with open(os.path.join(project, scene), "w", encoding="utf-8", newline="\n") as f:
        f.write(SCENE_3D if args.scene == "3d" else SCENE_2D)

    output = os.path.abspath(args.output)
    env = dict(os.environ, LATTICE_SHOT=output, LATTICE_SHOT_SCENE="res://" + scene)
    if args.crop:
        env["LATTICE_SHOT_CROP"] = args.crop
    # Imports first, or the scene opens before its resources are known.
    subprocess.run([editor, "--path", project, "--editor", "--headless", "--quit-after", "200"], env=dict(os.environ),
                   capture_output=True, timeout=300)
    result = subprocess.run([editor, "--path", project, "--editor"], env=env, capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=300)
    shutil.rmtree(project, ignore_errors=True)
    for line in (result.stdout + result.stderr).splitlines():
        if line.startswith("SHOT") or "ERROR" in line:
            print(line)
    ok = result.returncode == 0 and os.path.exists(output)
    print("OK:" if ok else "FAILED:", output)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
