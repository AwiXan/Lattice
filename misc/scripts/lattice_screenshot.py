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

def write_checker_png(path, size=512, cells=8):
    """A checker texture, for something textured to look at."""
    import struct
    import zlib

    rows = []
    for y in range(size):
        row = bytearray([0])
        for x in range(size):
            light = ((x * cells // size) + (y * cells // size)) % 2 == 0
            row += bytes((230, 180, 90) if light else (60, 90, 160))
        rows.append(bytes(row))
    raw = zlib.compress(b"".join(rows), 9)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", raw))
        f.write(chunk(b"IEND", b""))


def textured(scene):
    """The 3D scene with the crate and the floor wearing the checker texture."""
    scene = scene.replace('[sub_resource type="BoxMesh" id="box"]', '[ext_resource type="Texture2D" path="res://checker.png" id="1"]\n\n'
                          '[sub_resource type="StandardMaterial3D" id="checker"]\nalbedo_texture = ExtResource("1")\nuv1_scale = Vector3(2, 2, 2)\n\n'
                          '[sub_resource type="BoxMesh" id="box"]')
    for name in ("Crate", "Floor"):
        at = scene.index('[node name="%s"' % name)
        end = scene.index("mesh = ", at)
        end = scene.index("\n", end) + 1
        scene = scene[:end] + 'material_override = SubResource("checker")\n' + scene[end:]
    return scene


GI_RESOURCES = """[sub_resource type="StandardMaterial3D" id="red"]
albedo_color = Color(0.9, 0.1, 0.08, 1)

[sub_resource type="ProceduralSkyMaterial" id="sky_material"]

[sub_resource type="Sky" id="sky"]
sky_material = SubResource("sky_material")

[sub_resource type="Environment" id="gi_environment"]
background_mode = 2
sky = SubResource("sky")
tonemap_mode = 2
sdfgi_enabled = true

"""


GI_NODES = """
[node name="Wall" type="MeshInstance3D" parent="."]
transform = Transform3D(0.2, 0, 0, 0, 3, 0, 0, 0, 4, -1.6, 1.5, 0)
mesh = SubResource("box")
material_override = SubResource("red")

[node name="Sun" type="DirectionalLight3D" parent="."]
transform = Transform3D(0.7, -0.5, 0.5, 0, 0.7, 0.7, -0.7, -0.5, 0.5, 0, 6, 0)
shadow_enabled = true

[node name="Environment" type="WorldEnvironment" parent="."]
environment = SubResource("gi_environment")
"""


# --features: what the second batch of backports added, in one scene - all of
# it, or some (comma separated): contact (contact shadows from the sun), micro
# (microshadows), bounce (multi-bounce AO), decal (the checker, so with
# --textured), probe (a box-projected reflection probe), line (a Line3D), blur
# (motion blur in the camera attributes). The third batch's: ultra (Ultra soft
# shadows, PCF25 in Compatibility), minfov (the sun's minimum shadow FOV),
# compose (a sphere whose shader has compose()), lods (a SphereMesh with LODs
# and a shadow mesh), parallax (a deep-parallax tile), projector (a projector
# spot light without shadows), customcam (--camera with a custom projection),
# nodither (soft shadows without dithering).
FEATURES = ("contact", "micro", "bounce", "decal", "probe", "line", "blur",
            "ultra", "minfov", "compose", "lods", "parallax", "projector", "customcam", "nodither")

FEATURE_SETTINGS = {
    "contact": "lights_and_shadows/contact_shadow/enabled=true\n",
    "micro": "lights_and_shadows/micro_shadows/enabled=true\n",
    "bounce": "lights_and_shadows/multi_bounce_occlusion/enabled=true\n",
    "ultra": "lights_and_shadows/directional_shadow/soft_shadow_filter_quality=5\nlights_and_shadows/positional_shadow/soft_shadow_filter_quality=5\n",
    "nodither": "lights_and_shadows/soft_shadow_use_dithering=false\n",
}

FEATURE_RESOURCES = {
    "compose": """[sub_resource type="Shader" id="composing"]
code = "shader_type spatial;
void fragment() { ALBEDO = vec3(0.2, 0.6, 0.9); }
void compose() { DIFFUSE_COLOR = vec3(1.0, 0.0, 1.0) * (0.3 + DIFFUSE_LIGHT); SPECULAR_COLOR = SPECULAR_LIGHT; }
"

[sub_resource type="ShaderMaterial" id="composed"]
shader = SubResource("composing")

[sub_resource type="SphereMesh" id="composed_sphere"]
material = SubResource("composed")

""",
    "lods": """[sub_resource type="SphereMesh" id="lod_sphere"]
radius = 0.4
height = 0.8
radial_segments = 128
rings = 64
generate_lods = true
generate_shadow_mesh = true

""",
    "parallax": """[sub_resource type="StandardMaterial3D" id="deep"]
albedo_texture = ExtResource("1")
heightmap_enabled = true
heightmap_scale = 8.0
heightmap_deep_parallax = true
heightmap_correct_shadow_receive = true
heightmap_write_depth = true
heightmap_trim_edges = true
heightmap_texture = ExtResource("1")

[sub_resource type="PlaneMesh" id="deep_tile"]
material = SubResource("deep")
size = Vector2(2, 2)

""",
}

FEATURE_NODES = {
    "decal": """
[node name="Decal" type="Decal" parent="."]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 1.8, 0, 1.5)
size = Vector3(1.5, 1, 1.5)
texture_albedo = ExtResource("1")
""",
    "probe": """
[node name="Probe" type="ReflectionProbe" parent="."]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.9, 0)
size = Vector3(12, 4, 12)
box_projection = true
""",
    "line": """
[node name="Beam" type="Line3D" parent="."]
points = PackedVector3Array(-2, 0.3, 2, 0, 1.2, 2.5, 2, 0.3, 2)
""",
    "compose": """
[node name="Composed" type="MeshInstance3D" parent="."]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 1.6, 0.5, 1.2)
mesh = SubResource("composed_sphere")
""",
    "lods": """
[node name="LodSphere" type="MeshInstance3D" parent="."]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.4, 0.4, 2.2)
mesh = SubResource("lod_sphere")
""",
    "parallax": """
[node name="DeepTile" type="MeshInstance3D" parent="."]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 2.5, 0.01, -1.5)
mesh = SubResource("deep_tile")
""",
    "projector": """
[node name="Projector" type="SpotLight3D" parent="."]
transform = Transform3D(1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 3, 1)
light_energy = 4.0
light_projector = ExtResource("1")
spot_range = 6.0
""",
}

BLUR_RESOURCE = """[sub_resource type="CameraAttributesPractical" id="blurred"]
motion_blur_enabled = true

"""


CDB = r"C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe"


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
    parser.add_argument("--actions", help="what to open first, comma separated: sidebar, sidebar_page_<n>, debug_draw_<n> (16: GI cascades, 2: lighting)")
    parser.add_argument("--crashed", action="store_true", help="make the last session look crashed, with a long log")
    parser.add_argument("--lit", action="store_true", help="give the 3D scene a sun and an environment of its own")
    parser.add_argument("--camera", action="store_true", help="give the 3D scene a camera looking at the crate")
    parser.add_argument("--far", type=float, help="with --camera, its far distance (1e9: as good as unbounded)")
    parser.add_argument("--gi", action="store_true", help="give the 3D scene a sun, a sky, a red wall and real-time GI (SDFGI, or what replaced it)")
    parser.add_argument("--fog", action="store_true", help="with --gi, volumetric fog as well")
    parser.add_argument("--features", nargs="?", const="all", help="the second batch's features in the scene (implies --gi --textured): all, or some of " + ",".join(FEATURES))
    parser.add_argument("--select", help="the name of the node to select, instead of the first mesh")
    parser.add_argument("--streaming", action="store_true", help="turn texture streaming on in the project")
    parser.add_argument("--textured", action="store_true", help="put a checker texture on the 3D scene's crate and floor")
    parser.add_argument("--renderer", choices=["forward_plus", "mobile", "gl_compatibility"], help="the project's rendering method")
    parser.add_argument("--driver", choices=["vulkan", "d3d12"], help="the project's rendering device on Windows (default: vulkan)")
    parser.add_argument("--patch", action="append", default=[], help="'old=>new': a change to the scene's text before it is written, for trying one thing at a time (repeatable)")
    parser.add_argument("--editor-args", help="more arguments for the editor, in one string (say --accurate-breadcrumbs)")
    parser.add_argument("--debug", action="store_true", help="run the editor under cdb (Windows SDK) and print the stack if it crashes")
    parser.add_argument("--project", help="an existing project to open instead - a copy: the editor saves its state in it")
    parser.add_argument("--open", help="with --project, the scene to open (res://...)")
    parser.add_argument("--editor", help="editor binary to run (default: the newest one in bin/)")
    parser.add_argument("--timeout", type=int, default=60, help="seconds before calling it hung (it is left running)")
    parser.add_argument("--stress", type=int, default=0, help="seconds of opening and closing every dropdown first")
    parser.add_argument("--all", action="store_true", help="print everything the editor printed")
    parser.add_argument("--window", action="store_true", help="take the first window of its own showing - a dialog, a menu - instead")
    args = parser.parse_args()
    features = set()
    if args.features:
        args.gi = args.textured = True
        features = set(FEATURES) if args.features == "all" else set(args.features.split(","))

    editor = args.editor or find_editor()
    if not editor:
        print("No editor binary found; build one or pass --editor.")
        return 2

    if args.project:
        project = os.path.abspath(args.project)
        scene_path = args.open
    else:
        scene_path = None
        project = tempfile.mkdtemp(prefix="lattice-shot-")
        with open(os.path.join(project, "project.godot"), "w", encoding="utf-8", newline="\n") as f:
            f.write('config_version=5\n\n[application]\n\nconfig/name="Lattice screenshot"\n')
            if args.streaming or args.renderer or args.driver or args.features:
                f.write('\n[rendering]\n\n')
            for feature in FEATURES:
                if feature in features and feature in FEATURE_SETTINGS:
                    f.write(FEATURE_SETTINGS[feature])
            if args.streaming:
                f.write('textures/streaming/enabled=true\n')
            if args.renderer:
                f.write('renderer/rendering_method="%s"\n' % args.renderer)
            if args.driver:
                f.write('rendering_device/driver.windows="%s"\n' % args.driver)
        if args.textured:
            write_checker_png(os.path.join(project, "checker.png"))
        with open(os.path.join(project, "probe.gd"), "w", encoding="utf-8", newline="\n") as f:
            f.write("extends Node\n")
        scene = "scene_3d.tscn" if args.scene == "3d" else "scene_2d.tscn"
        scene_path = "res://" + scene
        with open(os.path.join(project, scene), "w", encoding="utf-8", newline="\n") as f:
            text = (textured(SCENE_3D) if args.textured else SCENE_3D) if args.scene == "3d" else SCENE_2D
            if args.gi and args.scene == "3d":
                resources = GI_RESOURCES
                if args.fog:
                    # GI lights the fog too (see volumetric_fog_process.glsl).
                    resources = resources.replace("sdfgi_enabled = true\n", "sdfgi_enabled = true\nvolumetric_fog_enabled = true\nvolumetric_fog_density = 0.03\n")
                if "blur" in features:
                    resources += BLUR_RESOURCE
                for feature in FEATURES:
                    if feature in features and feature in FEATURE_RESOURCES:
                        resources += FEATURE_RESOURCES[feature]
                text = text.replace('[sub_resource type="BoxMesh" id="box"]', resources + '[sub_resource type="BoxMesh" id="box"]')
            f.write(text)
            if args.lit and args.scene == "3d":
                # Its own sun and environment: the preview ones step aside and say so.
                f.write('\n[node name="Sun" type="DirectionalLight3D" parent="."]\n')
                f.write('\n[node name="Environment" type="WorldEnvironment" parent="."]\n')
            if args.gi and args.scene == "3d":
                # A red wall beside the crate, to see light bounce off it onto the
                # floor. "sdfgi_enabled" is read by whichever GI the build has.
                nodes = GI_NODES
                if "contact" in features:
                    nodes = nodes.replace("shadow_enabled = true\n", "shadow_enabled = true\nshadow_contact_shadows_allow = true\n")
                if "minfov" in features:
                    nodes = nodes.replace("shadow_enabled = true\n", "shadow_enabled = true\ndirectional_shadow_min_fov = 100.0\n")
                if "blur" in features:
                    nodes = nodes.replace('environment = SubResource("gi_environment")\n', 'environment = SubResource("gi_environment")\ncamera_attributes = SubResource("blurred")\n')
                for feature in FEATURES:
                    if feature in features and feature in FEATURE_NODES:
                        nodes += FEATURE_NODES[feature]
                f.write(nodes)
            if args.camera and args.scene == "3d":
                # Up and to the side, looking down at the crate.
                f.write('\n[node name="Camera" type="Camera3D" parent="."]\n')
                f.write('transform = Transform3D(0.8, -0.26, 0.54, 0, 0.9, 0.43, -0.6, -0.35, 0.72, 3, 2.5, 4)\n')
                if args.far:
                    f.write('far = %s\n' % repr(args.far))
                if "customcam" in features:
                    # A 60 degree perspective, wider than it is tall, given as a
                    # matrix rather than by fov.
                    f.write('projection = 3\ncustom_projection = Projection(0.974279, 0, 0, 0, 0, 1.73205, 0, 0, 0, 0, -1.00001, -1, 0, 0, -0.100001, 0)\n')

    if args.patch and not args.project:
        scene_file = os.path.join(project, scene)
        with open(scene_file, encoding="utf-8") as f:
            text = f.read()
        for change in args.patch:
            old, new = change.split("=>", 1)
            if old not in text:
                print("PATCH NOT FOUND:", old)
            text = text.replace(old.replace("\\n", "\n"), new.replace("\\n", "\n"))
        with open(scene_file, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)

    if args.crashed:
        editor_data = os.path.join(project, ".godot", "editor")
        os.makedirs(os.path.join(editor_data, "logs"), exist_ok=True)
        with open(os.path.join(editor_data, "session_running"), "w", encoding="utf-8") as f:
            f.write("999999")
        with open(os.path.join(editor_data, "logs", "editor.log"), "w", encoding="utf-8", newline="\n") as f:
            for i in range(300):
                f.write("A long line of the log, number %d, of a session that stopped half way.\n" % i)

    output = os.path.abspath(args.output)
    env = dict(os.environ, LATTICE_SHOT=output, LATTICE_SHOT_SCENE=scene_path)
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
    command = [editor, "--path", project, "--editor"] + (args.editor_args.split() if args.editor_args else [])
    if args.debug:
        # Under cdb, which prints the stack of a crash the engine's own handler
        # misses (one at exit, say). The editor itself, not its console wrapper,
        # which only starts it.
        command = [CDB, "-g", "-G", "-lines", "-c", "sxd av; g; .echo CRASH_STACK; .ecxr; kn 40; q",
                   editor.replace(".console.exe", ".exe"), "--path", project, "--editor"]
    process = subprocess.Popen(command, env=env, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace")
    try:
        out, _ = process.communicate(timeout=args.timeout)
    except subprocess.TimeoutExpired:
        # Left running, to be looked at with a debugger: it hung.
        print("HUNG: the editor did not finish; pid %d, project %s" % (process.pid, project))
        return 3
    if not args.project:
        shutil.rmtree(project, ignore_errors=True)
    crash = False
    for line in out.splitlines():
        crash = crash or "CRASH_STACK" in line
        if args.all or line.startswith("SHOT") or "ERROR" in line or crash:
            print(line)
    ok = process.returncode == 0 and os.path.exists(output)
    print("OK:" if ok else "FAILED (exit %d):" % process.returncode, output)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
