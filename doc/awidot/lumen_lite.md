# Lumen Lite: screen-probe global illumination on top of HDDAGI

Status: design, plus a first rough implementation (Forward+ only). Sections 2–5
describe what the first version does; where it is simpler than the design, the
text says so, and section 6 lists what is missing.

Lumen Lite is a second real-time GI mode next to HDDAGI. It keeps HDDAGI's
world-space scene representation and replaces only the *final gather*: instead
of interpolating HDDAGI's 8-cell probe grid at every pixel, it places probes
on the screen, traces rays from them (in screen space first, then through
HDDAGI's voxels), accumulates them over time, and interpolates those probes per
pixel. HDDAGI is unchanged and stays selectable; the two are mutually exclusive
through one mode switch on the Environment.

## 1. What HDDAGI already has

HDDAGI (`servers/rendering/renderer_rd/environment/gi.*`,
`shaders/environment/hddagi_*.glsl`, `gi.glsl`) keeps, per cascade (up to 8,
each 128 cells wide, cell size doubling per cascade, all centred on the camera
and scrolled in 8-cell regions):

| Data | Format | What it holds |
| --- | --- | --- |
| `voxel_region_tex` | `r8ui` 3D, one texel per 8³ region | "region has geometry" bit |
| `voxel_bits_tex` | `rg32ui` 3D, one texel per 4³ block | 64-bit occupancy mask of the block |
| `voxel_light_tex` | E5B9G9R9 3D (`r32ui` storage), one texel per cell | outgoing light of the empty cell in front of a surface: direct light (static + dynamic, shadowed by ray marching the voxels) + emission + `bounce_feedback` × albedo × probe irradiance (so it is multi-bounce) |
| `voxel_disocclusion_tex`, `voxel_light_neighbour_data` | `r8ui`, `r32ui` | which faces of a solid cell are open, which of the 26 neighbours are lit (used to filter hits) |
| `lightprobe_specular_tex` | E5B9G9R9 2D array, 5×5 octahedral + border per probe, one layer per cascade | per-direction radiance seen from each probe (the moving average of its rays, sky included when *Read Sky Light* is on) |
| `lightprobe_diffuse_tex` / `_filter_tex` | same layout | the cosine-convolved irradiance of the above |
| `occlusion_tex[2]` | R4G4B4A4 3D (`r16ui` storage) | per-cell visibility weights of the 8 surrounding probes (stops leaking through walls) |
| `hddagi_ubo` | UBO | grid size, cascade origins (camera-relative, world-aligned), `to_cell`, region offsets, energy, biases |

The scene-facing part is `gi.glsl` (`MODE_HDDAGI*`), dispatched from
`GI::process_gi()` in Forward+'s `_pre_opaque_render()`. Per pixel (at full,
half or lower resolution, `rendering/global_illumination/gi/...`), it

* finds the first cascade containing the pixel, interpolates the 8 surrounding
  probes' diffuse map in the normal's direction, weighted by trilinear ×
  normal facing × occlusion (`sdfvoxel_gi_process`);
* for glossy pixels (roughness < 0.25) traces one HDDA ray through the voxel
  occupancy (`trace_ray_hdda`) and reads `voxel_light_tex` at the hit,
  otherwise samples the probes' specular map;
* writes `ambient` and `reflection` (RGBE, `r32ui`) and a `rg8` blend buffer
  (how much of the scene's own ambient/reflection to replace — less than 1 in
  the fade at the edge of the last cascade).

These buffers are optionally bilateral-filtered and upsampled, and the Forward+
scene shader (`scene_forward_clustered.glsl`, `INSTANCE_FLAGS_USE_GI_BUFFERS`)
mixes them into `ambient_light` and `indirect_specular_light` before SSAO and
reflection probes are applied. Transparent and other forward-GI objects run
`hddagi_process()` themselves in the scene shader.

Where the quality falls short: the diffuse result is a probe grid of
8 cells (1.6 m at the default 0.2 m cell size in cascade 0) interpolated over
the screen. Contact bounce, small emitters, thin occluders and anything below
probe spacing are lost or leak, and the occlusion weights are coarse. The
traced voxels themselves are fine enough; it is the gather that throws detail
away. That is exactly the part Lumen replaces with screen probes.

## 2. Lumen Lite overview

```
                        HDDAGI world update (unchanged, read-only for us)
                         voxels, lit voxels, probe radiance cache
                                         |
 depth + normals ──> [1 place]──> [2 trace]──────> [3 filter + SH] ──> [4 gather] ──> GI buffers
 last frame color ──────────────────┘   ^  history        |                ^             (ambient, blend)
                                        └──────────────────┘                |
                                         filtered radiance (ping-pong)      └── probe SH irradiance
```

1. **Probe placement.** One screen probe per tile of 16×16 (Low/Medium) or
   8×8 (High) pixels of the internal resolution. Each frame the probe moves to
   a jittered pixel inside its tile (Halton-like hash of frame and tile, a few
   retries to avoid the sky). The probe stores its world position and normal.
   Tiles that only see sky get no probe.
2. **Trace.** Each probe owns an 8×8 octahedral radiance map over the full
   sphere (64 directions). One thread per texel traces one ray per frame,
   jittered inside the texel. Directions below the probe's surface are skipped.
   * **Screen-space trace first**: march the depth buffer from the probe
     (quadratic step distribution, 12–24 steps, thickness proportional to
     depth). On a hit, read the *previous frame's lit colour* at the hit,
     reprojected with last frame's view-projection. This gives contact bounce
     and detail of anything on screen, at no voxel resolution limit.
   * **World-space fallback**: if the ray leaves the screen, goes behind an
     object for longer than the thickness, or the reprojection fails, trace
     the same ray through HDDAGI's voxel occupancy with the existing
     `trace_ray_hdda()` (all cascades) and read `voxel_light_tex` at the hit,
     with HDDAGI's neighbour filtering. Read-only: Lumen Lite adds no voxel
     data of its own.
   * **Radiance cache for distant hits / misses**: when the world trace
     leaves the last cascade (or starts outside all of them, or hits the
     cell it started in), the ray reads
     HDDAGI's probe radiance (`lightprobe_specular`, interpolated with the
     occlusion weights) at its origin in its direction. These probes already
     include the sky, so this is also the sky term.
3. **Temporal + spatial filtering.** Each probe's new radiance is blended
   with the history radiance of the same tile from the previous frame, after
   checking that last frame's probe of that tile was on the same surface
   (world-space distance relative to depth, normal agreement). On a mismatch
   (disocclusion, camera cut) the history is dropped and the new trace stands
   alone. The blend is `1 - 1/frames` with a project setting for the frame
   count. Then a spatial pass averages each texel with the same direction of the
   four neighbouring probes, weighted by plane distance and normal similarity, which
   removes most of the per-ray noise before history is stored. The filtered map
   becomes next frame's history (ping-pong).
4. **Irradiance and gather.** The filter pass also projects each filtered
   octahedral map onto L1 spherical harmonics (4 coefficients × RGB) and
   convolves with the clamped cosine. The gather pass then evaluates, for every
   GI pixel, the four nearest probes (bilinear in tile space) at the pixel's
   normal, weighted by bilinear × plane-distance × normal agreement, and writes
   irradiance / π into the existing `ambient` GI buffer with blend 1. Pixels
   with no valid probe around keep HDDAGI's own value.

Importance sampling (planned, **not in the first version**): Lumen distributes a
probe's rays with the product of the BRDF (cosine around the pixel normals in
the tile) and the previous frame's radiance in the probe (directions that were
bright get more rays). The first version traces the octahedron uniformly with
per-frame jitter, which is noisier for small bright sources (an emissive lamp
covering a few texels). The plan:
reproject last frame's filtered probe radiance, build a 64-entry CDF per probe
in shared memory, and generate the 64 rays from it (structured importance
sampling, so each texel still gets at most one ray and the estimator weights
are `pdf_uniform / pdf`). Rays freed from directions below all pixel normals
of the tile go to the bright directions.

### Why build on HDDAGI's data

* The voxelisation, light injection, scrolling and multi-bounce feedback are
  the expensive, hard-won parts, and they are already correct and
  incrementally updated. A second global distance field would duplicate the
  scene representation and its memory (a 128³ × 4 cascade SDF + albedo is
  tens of MB).
* `trace_ray_hdda()` is a fast hierarchical DDA (regions → 4³ blocks →
  voxels) — effectively a distance-field skip — and returns the cell whose
  light we need.
* HDDAGI's probe grid is a world-space radiance cache with sky, occlusion and
  temporal accumulation already done: exactly what distant hits need.

So in Lumen Lite mode, HDDAGI's world update runs as usual and only the
per-pixel diffuse gather changes. Its settings (cascades, cell size, bounce
feedback, energy, sky) keep their meaning.

## 3. Working with the rest of the renderer

* **Reflections.** First version: the HDDAGI reflection path (one HDDA ray for
  glossy pixels, probe specular for rough ones) still writes the `reflection`
  buffer; SSR and reflection probes stay on top as today. Next step: rough
  reflections (roughness > 0.4) read the screen probes' radiance map in the
  reflection direction, and only glossy pixels trace, reusing the
  screen-then-world tracer.
* **SSAO.** Unchanged: the scene shader applies SSAO to the ambient after the
  GI buffers are mixed in. Screen probes already contain medium-scale
  occlusion, so recommend lowering SSAO intensity (or using it only for
  small-scale detail, small radius) when Lumen Lite is on.
* **SSIL.** Redundant with the screen trace (same idea, one bounce, no
  world fallback). It still works and adds on top; recommend disabling it with
  Lumen Lite. Later, Lumen Lite can switch it off by itself.
* **Lightmaps.** Objects using a baked lightmap or lightmap probes don't read
  the GI buffers (`#ifndef USE_LIGHTMAP`), as with HDDAGI. Their lit colour is
  in last frame's buffer, so they *emit* bounce into the screen trace and
  dynamic objects next to them pick it up. Static lights set to *bake static*
  are, as with HDDAGI, in the voxel light too.
* **VoxelGI.** As with HDDAGI, VoxelGI instances blend over the result in
  `gi.glsl` before Lumen Lite overwrites the ambient; inside a VoxelGI the
  Lumen ambient currently wins. To decide later.
* **Transparent objects** and anything rendered with forward GI keep reading
  HDDAGI's probes directly in the scene shader.
* **Fog / volumetric fog** keep reading HDDAGI's probes.
* **Multiview (XR)** is not supported yet: with two views the HDDAGI gather is
  used.

## 4. Quality tiers and cost

Settings live under `rendering/global_illumination/lumen_lite/` (read every
frame, so they apply live in the editor):

| Setting | Default | What it does |
| --- | --- | --- |
| `quality` | Medium | Low: 16 px tiles, 32 of the 64 rays per probe per frame (checkerboard; the others keep their history), 12 screen steps. Medium: 16 px, 64 rays, 16 steps. High: 8 px, 64 rays, 24 steps. |
| `history_frames` | 12 | The temporal blend weight of a new ray is `1 / history_frames`. |
| `screen_traces` | on | Off: rays go straight to the voxels (no contact detail, steadier). |
| `screen_trace_distance` | 4 m | How far a ray marches the depth buffer before the voxels take over. |
| `spatial_filter` | on | Averages each probe texel with the four neighbouring probes on the same plane. |

The mode itself is `Environment.dynamic_gi_mode` (HDDAGI or Lumen Lite) inside
the existing *Dynamic GI* group, so the two are exclusive by construction and
share `dynamic_gi_enabled`, the cascade settings and `dynamic_gi_energy`.

Expected cost at 1080p internal resolution on an RTX 3060-class GPU,
*estimated* from ray counts and the cost of the existing HDDAGI passes (not
measured yet; the first version runs in a software Vulkan rasterizer here):

| Tier | Probes | Rays / frame | Trace | Filter + SH | Gather (half res) | Total on top of HDDAGI's world update |
| --- | --- | --- | --- | --- | --- | --- |
| Low | 8 160 | 0.26 M | ~0.4 ms | ~0.05 ms | ~0.15 ms | **~0.6 ms** |
| Medium | 8 160 | 0.52 M | ~0.8–1.0 ms | ~0.08 ms | ~0.15 ms | **~1.0–1.3 ms** |
| High | 32 400 | 2.1 M | ~3–4 ms | ~0.3 ms | ~0.2 ms | **~3.5–4.5 ms** |

HDDAGI's world update (voxelisation of scrolled regions, light, probes) costs
the same as with HDDAGI itself, typically 0.5–1.5 ms; the HDDAGI per-pixel
gather still runs for reflections (~0.3 ms at half resolution).
Memory: per probe 8×8 RGBA16F × 3 maps + SH (3 × RGBA16F) + position and
normal for two frames ≈ 1.6 KB; Medium at 1080p ≈ 13 MB, High ≈ 52 MB, plus
the half-size copy of the lit scene (4 MB at 1080p) and, only while a debug view
is shown, a full-size debug texture.

The world fallback dominates the trace cost (a DDA through up to 8 cascades);
screen hits are cheap. In closed rooms most rays hit on screen.

## 5. Debug views

New `Viewport.DEBUG_DRAW_*` modes (also in the editor's *Display Advanced*
menu):

* **Lumen Lite Probes**: each tile shows its probe's filtered octahedral
  radiance map stretched over the tile, with the probe pixel marked.
* **Lumen Lite Trace Hits**: same layout, each texel coloured by how its last
  ray ended: green = screen-space hit, blue = voxel hit, red = radiance cache
  (miss / sky), black = not traced (below the surface).
* **Lumen Lite Radiance Cache**: per pixel, what the world radiance cache
  (HDDAGI probes) returns in the pixel's normal direction: the value a
  ray reaching that point would fall back to.

## 6. Roadmap and what the first version lacks

Known limitations of the first version:

* Uniform ray directions (no importance sampling): small bright emitters give
  blotchy noise that the 4-neighbour filter and 12-frame history don't fully
  hide.
* Screen trace is a linear march of the full depth buffer, no HZB; rays that
  go behind thin objects or off screen fall back to voxels.
* History is matched per probe (tile reprojection + plane test), not per
  texel; fast camera motion resets it, dynamic objects get no motion vectors.
* No adaptive extra probes at depth/normal edges; the gather falls back to
  any facing probe in the 3×3 tiles, else to HDDAGI's own value.
* Reflections, transparent objects, fog and multiview stay on HDDAGI.
* Settings are project-wide (read every frame); there is no per-Environment
  quality and no RenderingServer setter beyond the mode.
* The editor's settings update in `editor_node.cpp` is untouched; the
  settings apply live anyway because they are read with `GLOBAL_GET_CACHED`.

Roadmap:


1. (this PR) Screen probes, screen → voxel → cache tracing, temporal +
   spatial filter, SH gather, settings, debug views; Forward+ only.
2. Importance sampling from last frame's probe radiance and the tile's normals.
3. Hierarchical-Z screen trace (cheaper, longer rays, fewer misses).
4. Rough reflections from probe radiance; glossy reflections with the
   same tracer, denoised.
5. Adaptive probe placement (extra probes in tiles with depth or normal
   discontinuities, as Lumen does), and per-pixel bent-normal / short-range
   AO from the screen trace.
6. Reprojection with motion vectors for dynamic objects; history
   confidence per texel.
7. Mobile renderer port (cheaper tier: 32 px tiles, world fallback only), and
   multiview.
8. Turning SSIL off automatically and tuning SSAO when Lumen Lite is on.
