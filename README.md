# Tic Tac Toe — Why2025 App

Small Tic Tac Toe game intended as an app for the Why2025 Team Badge firmware.

- Firmware project: https://gitlab.com/why2025/team-badge/firmware
- App ID: `tic_tac_toe` (see `manifest.json`)

## Overview

This app lets you play Tic Tac Toe against a simple algorithm. It includes a lightweight desktop build (SDL3) for quick iteration and a manifest for integration into the Why2025 badge firmware build.

### Controls

- Arrow keys: Move selection
- Space/Enter: Place your move
- R: Restart game
- Mouse: Click a cell to place your move
- Esc: Quit (desktop build)

## Run locally (desktop, macOS/Linux)

Requirements:

- clang or gcc
- pkg-config
- SDL3 development package

Example on macOS (Homebrew):

```bash
brew install sdl3 pkg-config
clang tic_tac_toe.c -o tic_tac_toe $(pkg-config --cflags --libs sdl3)
./tic_tac_toe
```

Notes:

- The desktop build uses SDL3’s callback entry points (SDL_AppInit/Iterate/Event/Quit); no `main()` is needed.
- The provided `CMakeLists.txt` is for ESP-IDF component registration in the badge firmware; it isn’t required for the desktop build above.

## Integrate with Why2025 firmware

The badge firmware repository orchestrates building and packaging of apps. This repo includes `manifest.json` so it can be picked up by that build.

Typical steps (high level):

1. Clone the firmware repository and follow its setup guide.
2. Add this app into the firmware’s apps workspace (e.g., as a submodule or by copying this folder).
3. Build the firmware/apps per the firmware repo instructions; ensure the output ELF is named `tic_tac_toe.elf` to match `binary_path` in `manifest.json`.
4. Flash or deploy as described in the firmware README.

Because app build/integration details are controlled by the firmware project, consult that repo’s README for exact paths, commands, and supported targets.

## Repository layout

- `tic_tac_toe.c` — Game logic and SDL3 rendering
- `tic_tac_toe.h` — App entry declaration
- `manifest.json` — App metadata for Why2025 firmware tooling
- `CMakeLists.txt` — ESP-IDF component registration (used when building inside the firmware)
- `storage_skel/` — Placeholder for any app-specific storage layout (if used by the firmware tooling)

## Manifest

`manifest.json` fields used here:

- `unique_identifier`: Stable ID for the app (`tic_tac_toe`).
- `name`: Human-friendly name.
- `binary_path`: Expected output ELF name (`tic_tac_toe.elf`).
- `source`: Indicates this app is built from source by the firmware tooling.

## Troubleshooting

- Desktop build: If `pkg-config --cflags --libs sdl3` fails, ensure SDL3 is installed and that pkg-config can find `sdl3.pc`.
- Firmware build: If the app isn’t discovered, verify the app folder structure and `manifest.json` placement as required by the firmware repo.

## License

This project is licensed under the Zero-Clause BSD (0BSD) license. You may use, copy, modify, and/or distribute for any purpose with or without fee. Provided “as is”, without warranty or liability. See `LICENSE`.

