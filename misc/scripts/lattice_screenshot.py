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
    parser.add_argument("--actions", help="what to open first, comma separated: sidebar, sidebar_page_<n>")
    parser.add_argument("--crashed", action="store_true", help="make the last session look crashed, with a long log")
    parser.add_argument("--lit", action="store_true", help="give the 3D scene a sun and an environment of its own")
    parser.add_argument("--camera", action="store_true", help="give the 3D scene a camera looking at the crate")
    parser.add_argument("--select", help="the name of the node to select, instead of the first mesh")
    parser.add_argument("--streaming", action="store_true", help="turn texture streaming on in the project")
    parser.add_argument("--editor", help="editor binary to run (default: the newest one in bin/)")
    parser.add_argument("--timeout", type=int, default=60, help="seconds before calling it hung (it is left running)")
    parser.add_argument("--stress", type=int, default=0, help="seconds of opening and closing every dropdown first")
    parser.add_argument("--all", action="store_true", help="print everything the editor printed")
    parser.add_argument("--window", action="store_true", help="take the first window of its own showing - a dialog, a menu - instead")
    args = parser.parse_args()

    editor = args.editor or find_editor()
    if not editor:
        print("No editor binary found; build one or pass --editor.")
        return 2

    project = tempfile.mkdtemp(prefix="lattice-shot-")
    with open(os.path.join(project, "project.godot"), "w", encoding="utf-8", newline="\n") as f:
        f.write('config_version=5\n\n[application]\n\nconfig/name="Lattice screenshot"\n')
        if args.streaming:
            f.write('\n[rendering]\n\ntextures/streaming/enabled=true\n')
    with open(os.path.join(project, "probe.gd"), "w", encoding="utf-8", newline="\n") as f:
        f.write("extends Node\n")
    scene = "scene_3d.tscn" if args.scene == "3d" else "scene_2d.tscn"
    with open(os.path.join(project, scene), "w", encoding="utf-8", newline="\n") as f:
        f.write(SCENE_3D if args.scene == "3d" else SCENE_2D)
        if args.lit and args.scene == "3d":
            # Its own sun and environment: the preview ones step aside and say so.
            f.write('\n[node name="Sun" type="DirectionalLight3D" parent="."]\n')
            f.write('\n[node name="Environment" type="WorldEnvironment" parent="."]\n')
        if args.camera and args.scene == "3d":
            # Up and to the side, looking down at the crate.
            f.write('\n[node name="Camera" type="Camera3D" parent="."]\n')
            f.write('transform = Transform3D(0.8, -0.26, 0.54, 0, 0.9, 0.43, -0.6, -0.35, 0.72, 3, 2.5, 4)\n')

    if args.crashed:
        editor_data = os.path.join(project, ".godot", "editor")
        os.makedirs(os.path.join(editor_data, "logs"), exist_ok=True)
        with open(os.path.join(editor_data, "session_running"), "w", encoding="utf-8") as f:
            f.write("999999")
        with open(os.path.join(editor_data, "logs", "editor.log"), "w", encoding="utf-8", newline="\n") as f:
            for i in range(300):
                f.write("A long line of the log, number %d, of a session that stopped half way.\n" % i)

    output = os.path.abspath(args.output)
    env = dict(os.environ, LATTICE_SHOT=output, LATTICE_SHOT_SCENE="res://" + scene)
    if args.crop:
        env["LATTICE_SHOT_CROP"] = args.crop
    if args.actions:
        env["LATTICE_SHOT_ACTIONS"] = args.actions
    if args.select:
        env["LATTICE_SHOT_SELECT"] = args.select
    if args.stress:
        env["LATTICE_STRESS_POPUPS"] = str(args.stress)
    if args.crashed or args.window:
        # Its report is what is worth a picture.
        env["LATTICE_SHOT_WINDOW"] = "1"
    # Imports first, or the scene opens before its resources are known.
    subprocess.run([editor, "--path", project, "--editor", "--headless", "--quit-after", "200"], env=dict(os.environ),
                   capture_output=True, timeout=300)
    process = subprocess.Popen([editor, "--path", project, "--editor"], env=env, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace")
    try:
        out, _ = process.communicate(timeout=args.timeout)
    except subprocess.TimeoutExpired:
        # Left running, to be looked at with a debugger: it hung.
        print("HUNG: the editor did not finish; pid %d, project %s" % (process.pid, project))
        return 3
    shutil.rmtree(project, ignore_errors=True)
    for line in out.splitlines():
        if args.all or line.startswith("SHOT") or "ERROR" in line:
            print(line)
    ok = process.returncode == 0 and os.path.exists(output)
    print("OK:" if ok else "FAILED:", output)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
