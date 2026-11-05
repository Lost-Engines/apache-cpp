# Apache C++

A native C++26 reconstruction of Digital Integration's **Apache Longbow**, released as **Apache** in the US, from the [Lost Engines](https://github.com/Lost-Engines) reverse-engineering project.

Quickstart, training, single missions and campaigns are playable, with the original software graphics, flight models, combat, instruments, mission planner, music and speech. LAN multiplayer includes combat, cooperative missions and a shared pilot/gunner cockpit.

## Getting started

[Download the latest release](https://github.com/Lost-Engines/apache-cpp/releases/latest) for Linux, Windows or macOS and unpack it.

On Windows, run `scripts\fetch-assets.bat`, then open `apache.exe`. On Linux or macOS, run these commands inside the unpacked directory:

```sh
./scripts/fetch-assets.sh
./apache
```

The fetch helper downloads the reference CD from Archive.org, verifies its checksum and extracts the game files. The temporary CD image is removed afterwards. On Windows, use `scripts\fetch-assets.bat`. If your executable is somewhere other than `build/` or the package directory, set `APACHE_EXECUTABLE` to its path.

Already have the game? Select an extracted CD directory or a matching Windows software installation:

```sh
./apache --data-dir /path/to/apache
```

That location is remembered for subsequent launches. The reference is the January 1998 software build, supplied on the Sold Out and October 2000 European CDs. Other engine versions are not supported yet. Original files are read without modification.

## Playing

Flight opens the training, single-mission and campaign choices. Briefing maps support route editing, payload selection and a live 3D preview. Use Preferences to choose graphics, flight model, difficulty, audio and keyboard/joystick controls.

Arrow keys control pitch and roll, Q/A change collective, Enter selects a weapon, and Space fires. F2/F3 select the pilot/gunner cockpit. F1 opens the illustrated manual in single-player flight. P or Pause pauses; Ctrl+Escape asks whether to end the sortie (Y/N).

The window uses square pixels and nearest-neighbour scaling. `--skip-intro` skips the opening sequence, and `--no-audio` disables playback. Run `apache --help` for other options.

Pilot profiles and preferences are saved under `$XDG_DATA_HOME/apache` or `~/.local/share/apache` on Linux, and `%LOCALAPPDATA%\apache` on Windows. `--save-dir` selects another location. Fetched game files occupy the separate `game` subdirectory on Linux, or `data` beside the Windows executable.

## Multiplayer

Two Player offers combat, cooperative missions and a shared pilot/gunner cockpit. Network supports combat with up to eight Apache players. Players on the same LAN are discovered automatically; a host address can also be entered in the menu. Everyone must choose the same network channel. These sessions connect Apache C++ copies using UDP, rather than the original IPX or modem drivers.

Press Shift+M during flight to chat, Enter to send, and Up to recall the last message. Pause and the manual are disabled during network flight, as in the original.

Capture the Flag needs the complete `MISSDATA/CAPTURE.DTA` from the US or Japanese CD. The reference European disc is missing the flag and respawn tasks; the menu explains this before takeoff.

## Building from source

Use CMake 3.30 or later, a C++26 compiler and OpenGL development libraries. The [build guide](docs/build-release.md) lists platform dependencies.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j3
ctest --test-dir build --output-on-failure
./scripts/fetch-assets.sh
./build/apache
```

`apache_inspect` exercises the decoded resources and simulation without opening a window. See [implementation and validation](docs/implementation.md) for the original-code comparisons and release checks. Game data and generated captures are kept outside this repository.
