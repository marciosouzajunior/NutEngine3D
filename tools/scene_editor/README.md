# NutSceneEditor

Desktop scene editor for the authored `.nutscene` workflow.

## What it does

- loads authored scenes from `assets/scenes/`
- saves edited `.nutscene` JSON back to disk
- runs the shared scene analysis used by the CLI compiler
- compiles the active scene into:
  - `build/assets/nano/demo_scene.h`
  - `build/assets/nano/demo_scene_limits.h`
- builds and uploads the Nano firmware through PlatformIO
- builds and launches the desktop simulator from the same compiled scene

## Supported scenes

The current in-editor scene switcher supports:

- `assets/scenes/demo.nutscene`
- `assets/scenes/tunnel_run.nutscene`

The selected scene is persisted in:

- `tools/scene_editor/editor_user_settings.json`

## Build modal flow

The editor exposes one `Build` entry point. From that modal you can run:

- `Compile Scene`
- `Build Nano`
- `Upload Nano`
- `Build & Upload`
- `Run Desktop`

The flow is always staged:

1. save the scene when there are unsaved changes
2. analyze the scene with the shared compiler pipeline
3. compile the scene headers
4. continue to Nano build/upload or desktop run only if the previous step succeeded

Compiler and PlatformIO output is captured and shown inside the modal.

## Nano limits and clean builds

The generated `demo_scene_limits.h` drives the active Nano capacities through the
normal runtime-limits include chain.

When the editor detects that the generated limits file changed, it automatically
runs `PlatformIO clean` before `Build Nano`, `Upload Nano`, or `Build & Upload`.
This keeps scene switches such as `demo -> tunnel_run -> demo` reliable without
manual cleanup.

## Toolchain settings

The build modal lets each developer configure:

- `PlatformIO Path`
- `Upload Port`

Those values are stored locally in:

- `tools/scene_editor/editor_user_settings.json`

They are user-specific settings and should not be committed.

## Script source preview

The inspector can open a read-only source preview for scripts under
`assets/scripts/`. This is a preview surface, not a full code editor.

## Build

From the repository root:

```bat
tools\scene_editor\build.bat
```

Run:

```bat
tools\scene_editor\run.bat
```
