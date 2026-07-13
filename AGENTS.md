# NutEngine3D Engineering Guide

This file records the constraints and lessons that must guide changes to this
repository. The Arduino Nano is the primary runtime target. Desktop exists to
make authoring, simulation, and debugging easier; it must not drive the core
architecture toward a design the Nano cannot sustain.

## Primary hardware budget

The validated target is an Arduino Nano with an ATmega328P at 16 MHz:

- 2 KB SRAM
- 30 KB usable program flash
- SSD1306 128x64 OLED at address `0x3C`
- I2C on `SDA=A4`, `SCL=A5`
- paged display output with one 128-byte page buffer
- I2C clock currently validated at 700 kHz on this hardware and wiring

Treat SRAM, not flash, as the first constraint. Flash usually has useful room;
SRAM must hold globals, fixed containers, library state,
serial buffers, the active display page, and the live stack.

The stable four-child stress build originally measured:

- RAM: 1758/2048 bytes (85.8%)
- flash: 17168/30720 bytes (55.9%)

This is already a tight SRAM budget. The remaining 290 bytes are not freely
available: stack depth and temporary values need part of that margin. A build
that links successfully can still corrupt memory or reboot on hardware.

After sharing the scene-loader scratch with the renderer caches, and including
the per-frame joystick input state, the current builds measure:

- `nano`: 1532/2048 bytes RAM (74.8%)
- `nano_stress`: 1464/2048 bytes RAM (71.5%)
- `nano_stress4`: 1534/2048 bytes RAM (74.9%)
- `nano_stress4_logs`: 1538/2048 bytes RAM (75.1%)

The loader/renderer optimization recovered 224 bytes in `nano_stress4` relative
to the previous 1758-byte build. Treat these as the new reference numbers.

## Memory rules

1. Do not introduce ordinary runtime heap allocation on the Nano path.
   Avoid `std::vector`, owning `std::string`, `std::map`, `malloc`, and normal
   `new` in frame, update, render, input, or scene-transition code.
2. Prefer fixed-capacity storage (`FixedVector`, arrays, compact indexes, and
   target build limits). Every capacity consumes SRAM even when empty, so do
   not increase a limit without measuring the resulting binary.
3. Placement-new into preallocated, correctly aligned storage is acceptable.
   `RuntimeScene` uses this for Nano `GameObject` instances.
4. `NanoNoHeap.cpp` enforces the heapless contract and prevents Arduino's
   default allocator from being linked. Ordinary `new` aborts, nothrow `new`
   returns `nullptr`, and placement-new remains available for preallocated
   storage. Do not weaken this guard to make a feature compile.
5. Keep large immutable data in flash (`PROGMEM`). Read scene and mesh bytes
   from the compiled blob instead of copying geometry into SRAM.
6. Prefer small integer indexes and offsets for serialized/runtime references.
   Be explicit about widths and overflow. The AVR has 16-bit `int` and
   pointers; desktop assumptions about integer size are unsafe here.
7. Avoid recursion on Nano. Stack usage is difficult to budget and previously
   caused corruption during scene rendering. Use bounded iterative traversal.
8. Do not optimize one structure in isolation. Always account for global SRAM,
   stack headroom, Wire/Serial state, and temporary call-frame cost together.

If a new feature needs persistent memory, first state its byte cost at maximum
capacity. If the cost is not known, measure it before merging the design.

An A/B build of `nano_stress4` originally confirmed why the custom global
operators must remain in the Nano build:

- with custom `new/delete`: 1758 bytes RAM, 17168 bytes flash
- with Arduino's default implementation: 1768 bytes RAM, 17778 bytes flash

Keep `NanoNoHeap.cpp` in every Nano environment. Removing it currently costs 10
bytes of RAM and 610 bytes of flash because Arduino's allocator implementation
is linked instead. If a future feature requires dynamic allocation, redesign
the ownership/storage first rather than adding an arena silently.

## CPU versus RAM

On this target, recomputing can be safer than caching. A useful cache on a
desktop can remove the Nano's stack margin.

The renderer currently caches projected vertices once per frame, then rereads
compact edge indexes from the scene blob for each OLED page. An experiment also
cached up to 80 visible edges. It reduced repeated CPU work but added 162 bytes
of static SRAM: usage rose from 85.8% to 93.8%. The LCD showed garbage and the
board rebooted. Removing that cache restored stability.

Use this decision order for caches:

1. Estimate exact worst-case bytes, including size/capacity fields and padding.
2. Check whether the source can be reread cheaply from flash.
3. Prefer compact derived metadata over duplicated geometry.
4. Build clean and record RAM/flash before testing performance.
5. Test on the real Nano; desktop success does not validate SRAM or stack.
6. Reject an optimization that improves CPU but removes safe runtime margin.

When CPU work is repeated across all eight display pages, a cache may still be
worth considering, but it must be smaller than the saved risk justifies. Look
first for bit packing, page-span metadata, cheaper rejection, mesh simplification,
or doing less work rather than storing a complete second representation.

## Display and render pipeline

The SSD1306 is rendered in eight horizontal pages, each 8 pixels high. The Nano
keeps only one 128-byte page in SRAM:

1. `beginFrame()` measures elapsed time.
2. scripts update transforms once for the logical frame.
3. `NanoWireframeRenderer::prepareFrame()` traverses the scene iteratively and
   transforms/projects vertices once into a compact cache.
4. For each of 8 pages, the page buffer is cleared.
5. `drawCachedLines()` reads edges, rejects invisible/offscreen lines, and
   rasterizes only pixels belonging to the active page.
6. `present()` sends that 128-byte page over I2C.

One visual frame means all eight pages have been drawn and transmitted. Do not
update object transforms between pages or the top and bottom of the display can
represent different moments.

Earlier instrumentation at 700 kHz measured approximately:

- frame preparation: about 13 ms
- line drawing across pages: about 34-42 ms depending on orientation
- OLED transmission: about 29.5 ms
- total frame: about 77-86 ms in the measured demo/stress conditions

These are reference measurements, not guarantees. Geometry, clipping, logs,
and instrumentation change them. Present time is a major fixed cost; simplifying
meshes reduces draw time but does not remove the eight page transfers.

## Scene and hierarchy constraints

The authored `.nutscene` JSON is the source of truth. `NutSceneCompiler`
generates `build/assets/nano/demo_scene.h`, which contains the binary scene blob
as a `PROGMEM` byte array. The Nano loads that binary directly; it does not parse
JSON and has no runtime filesystem.

Use:

```bat
tools\scene_compiler\run_scene.bat assets\scenes\demo.nutscene
```

Then build/upload the desired PlatformIO environment. Remember that compiling
firmware does not necessarily regenerate the scene header. When scene behavior
looks stale, regenerate the header, clean the environment, rebuild, and verify
the firmware tag over serial.

Nano hierarchy is intentionally limited to:

- root objects
- one direct child level
- no grandchildren

The scene compiler should reject deeper hierarchies. The Nano renderer relies
on this rule and uses iterative root/child traversal. Do not reintroduce a
generic recursive walk without a measured stack budget and a compelling need.

Runtime capacities come from `src/core/RuntimeLimits.h` and can be overridden
per PlatformIO environment with `NUT_CFG_*` build flags. Limits are contracts,
not goals. Raise only the capacities required by the scene being tested.

Current real-hardware observations:

- the normal demo is stable and acceptably fluid
- three rotating child cubes are smooth enough for the stress test
- four rotating child cubes work but are visibly slower
- four visible cubes with three scripts (Spin, PlayerMove, and Dummy) plus
  joystick input are acceptable at 1586 bytes RAM (77.4%)
- two cubes plus two low-poly spheres (40 projected vertices, approximately 84
  edges), three scripts, and input are acceptable at 1604 bytes RAM (78.3%)
- four low-poly spheres (48 projected vertices, approximately 120 edges), three
  scripts, and input are acceptable at 1604 bytes RAM (78.3%)
- five visible cubes with one shared mesh and one parent script are acceptable
  at 1656 bytes RAM (80.9%), but leave less CPU headroom
- five visible cubes with three scripts run but are not smooth at 1708 bytes
  RAM (83.4%); do not use this as the normal gameplay budget

Use four simple visible objects, one shared mesh where possible, and roughly
three active script instances as the current recommended Nano gameplay budget.
Five visible objects are a tested upper bound, not a comfortable design target.
For geometry, 48-84 rendered edges per frame is the comfortable tested range;
approximately 120 edges is acceptable on the current demo but should be treated
as the general upper budget. A 60-vertex/146-edge scene with four visible
objects, three scripts, and input also runs, but motion is noticeably strained;
reserve that density for games or scenes that do not require fast animation.

Object count alone is not the real cost. Vertices, edges, scripts, hierarchy,
input, future collision/physics, and display traffic share the same CPU/RAM
budget. Prefer shared meshes and simple wireframes.

An experiment clipped each line to the active 8-pixel OLED page before running
Bresenham. It was rejected: integer endpoint rounding restarted Bresenham with a
different error phase on each page and left visible gaps in diagonal edges at
page boundaries. Keep the cheap page bounding-box rejection, but rasterize from
the original endpoints unless a future clipper explicitly preserves the line's
rasterization phase. The renderer still reuses a parent's rotation trig for
children whose local rotation is zero. That change reduced flash slightly and
remained stable on hardware, but its visual performance gain was inconclusive.

### Loader/renderer shared SRAM

On Nano, `main.cpp` owns one phase storage union. During `setupScene()` it is
the `SceneBinaryLoader` scratch; after loading finishes, placement-new constructs
`NanoWireframeRenderer` over the same bytes. Never use the loader and renderer
at the same time. Before a future scene reload, destroy the renderer, run the
loader, and reconstruct the renderer only after load succeeds.

The decoded string table is intentionally not part of this shared block because
Nano `GameObject` names point into it for the scene lifetime. Loader record types
must stay free of default member initializers: AVR otherwise emits a large
SRAM-backed initialization template. Every record field is decoded before use,
and counters define the valid ranges. `NANO_SCRATCH_BYTES` is protected by a
compile-time size assertion; do not bypass it when increasing runtime limits.

## Input

Input is sampled once per logical frame before script updates. The current Nano
path reads joystick X/Y axes and one pull-up button into the compact `InputState`
stored by `Scene`. Do not read hardware independently from each script or once
per OLED page. New controls should extend the snapshot compactly and their RAM,
ADC time, and per-frame CPU cost must be included in stress measurements.

## Scripts

Scripts are native C++ in `assets/scripts`; they are compiled into firmware.
The scene binary stores script IDs plus compact property/config data, not machine
code. Keep the Nano script model fixed-size and direct. Avoid registries,
factories, polymorphic ownership, or per-instance heap allocation unless their
cost is demonstrated to be lower than the current mechanism.

Dynamic script properties are allowed in the sense that each scene instance can
carry different compiled configuration values. They must deserialize into the
bounded core `CompiledScriptInstance` payload. Do not interpret raw JSON at
runtime.

`RuntimeScene` and `SceneBinaryLoader` must remain independent of concrete game
scripts. The loader copies opaque config bytes; modules in `assets/scripts`
interpret them, and `GameScriptDispatcher` is the game-side composition point.

Update scripts once per logical frame, before rendering any OLED page. Keep
per-frame script code allocation-free and avoid expensive floating-point work in
inner loops when a compact or precomputed representation is practical.

## Logging and instrumentation

Serial logging is essential, but it changes the program being observed.
Additional literals, formatting, calls, local variables, and stack frames have
previously moved failures or made the LCD stop working.

Follow these rules:

1. Keep the normal firmware quiet. Put heavy diagnostics behind a dedicated
   build flag/environment such as `NUT_ENABLE_STRESS_LOGS`.
2. Store constant log strings in flash with `F("...")` on Arduino.
3. Prefer short stage markers (`T0`, `P1`) while locating a crash. Expand only
   the last confirmed interval.
4. Do not print inside edge, vertex, pixel, or page inner loops. Accumulate
   bounded counters and print one summary after many frames.
5. Rate-limit performance reports. Serial at 115200 can itself become a timing
   and buffering cost.
6. Avoid `printf`-style formatting and temporary `String` objects on Nano.
   Use direct `Serial.print()` calls with fixed-width values.
7. Treat corrupted serial text, NUL characters, impossible counts, OLED garbage,
   repeated boot logs, and intermittent OLED initialization failures as likely
   SRAM/stack corruption until disproven.
8. A log disappearing identifies the interval of failure, not necessarily the
   exact faulty statement. Adding the next log changes memory and timing.
9. Give every experimental firmware a visible serial tag via `NUT_FW_TAG`.
   If the tag did not change, do not trust that the new firmware is running.
10. Close the serial monitor before upload when COM5 is busy. Retry transient
    Nano bootloader sync failures before changing upload configuration.

For timing, accumulate integer microseconds/counters and report averages at a
low frequency. Instrument preparation, drawing, and presentation separately.
First verify that instrumented rendering still appears correctly on the LCD;
otherwise the measurement is not representative.

## Debugging protocol

When hardware becomes unstable:

1. Return to the last known-good scene/environment and confirm the hardware.
2. Record the firmware tag, PlatformIO environment, RAM, and flash usage.
3. Clean-build before assuming source changes were included.
4. Separate scene load, script update, frame preparation, page drawing, and
   page presentation with minimal stage markers.
5. Remove recursion and large automatic arrays before chasing binary parsing.
6. Validate counts and indexes before dereferencing scene data. Print unsigned
   values with appropriate types; misleading signed output can resemble memory
   corruption.
7. Change one variable at a time: object count, mesh density, cache size, I2C
   speed, logging, or runtime capacity.
8. After finding the cause, remove diagnostic code and retest the clean build.

The most useful comparison is always a known-good firmware on the same board,
display, wiring, scene header, and PlatformIO environment.

## Build and test expectations

- PlatformIO under `targets/nano` is the canonical Nano build path.
- Use the environment that matches the generated scene limits.
- Run a clean build after changing headers, build flags, generated scene data,
  or when observed firmware does not match the source.
- Always report PlatformIO RAM and flash percentages for Nano changes that add
  state, containers, libraries, scripts, or renderer caches.
- Validate meaningful runtime changes on the physical LCD, not only by build
  success or serial logs.
- Preserve a known-good baseline environment while using separate stress
  environments for limit exploration.
- Desktop simulation should use the same scene format, script model, transforms,
  and behavioral contracts where practical, but desktop-only convenience must
  stay out of the Nano binary.

### Tests still required

The current numbers are useful design guidance, not a complete capacity model.
Before declaring final game limits, run controlled tests that vary one cost at a
time and record firmware tag, RAM, flash, frame time, visual quality, and resets:

1. Soak the normal demo and the recommended four-object/three-script scene for
   at least 30 minutes with continuous joystick input.
2. Measure stack headroom at scene load, script update, frame preparation, page
   drawing, and I2C presentation without leaving heavy instrumentation enabled.
3. Sweep object count using one tiny shared mesh so object/runtime cost is
   separated from geometry cost.
4. Sweep vertices and edges independently, including long lines and mostly
   off-screen geometry, while checking continuity at every OLED page boundary.
5. Sweep active script instances and script property bytes independently; test
   scripts that update transforms as well as scripts that remain idle.
6. Measure joystick sampling plus button handling under sustained movement and
   verify that input does not introduce visible frame pacing changes.
7. Test scene-load rejection exactly at and one item beyond every configured
   object, mesh, script, child, string-table, and loader-scratch limit.
8. Add a minimal collision/input/gameplay prototype before reserving a physics
   budget. Rendering-only benchmarks cannot prove that a complete game fits.
9. Re-run the recommended scene at 400 kHz I2C as a compatibility baseline and
   keep 700 kHz documented as validated for the current display and wiring.
10. Validate desktop simulation against the same generated scene after loader,
    transform, script, input, or binary-format changes.

## Design priorities

In order:

1. deterministic behavior and memory safety on real Nano hardware
2. enough SRAM headroom for the live stack and future gameplay features
3. coherent motion and stable OLED output
4. bounded scene/script behavior shared with desktop simulation
5. CPU optimizations that do not sacrifice memory safety
6. additional object count or visual detail

Do not use successful compilation as proof that a design fits. On this project,
the final authority is a clean, tagged build running continuously on the real
Nano with a correct LCD image and stable serial output.
