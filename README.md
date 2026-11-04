# Apache C++

A native C++26 reconstruction of Digital Integration's **Apache Longbow**, released as **Apache** in the US, from the [Lost Engines](https://github.com/Lost-Engines) reverse-engineering project.

Quickstart, training, single missions and campaigns are playable, with the original software graphics, flight models, combat, instruments, mission planner, music and speech. Development continues on multiplayer and final verification.

## Getting started

Build with CMake 3.30 or later, a C++26 compiler and OpenGL development libraries:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
./scripts/fetch-assets.sh
./build/apache
```

The fetch helper downloads the reference CD from Archive.org, verifies its checksum and extracts the game files. The temporary CD image is removed afterwards. On Windows, use `scripts\fetch-assets.bat`. If your executable is somewhere other than `build/` or the package directory, set `APACHE_EXECUTABLE` to its path.

Already have the game? Select an extracted CD directory or a matching Windows software installation:

```sh
./build/apache --data-dir /path/to/apache
```

That location is remembered for subsequent launches. The reference is the January 1998 software build, supplied on the Sold Out and October 2000 European CDs. Other engine versions are not supported yet. Original files are read without modification.

## Playing

Flight opens the training, single-mission and campaign choices. Briefing maps support route editing, payload selection and a live 3D preview. Preferences controls graphics, flight model, difficulty, audio and keyboard/joystick selection.

Arrow keys control pitch and roll, Q/A change collective, Enter selects a weapon, and Space fires. F2/F3 select the pilot/gunner cockpit. F1 opens the illustrated manual, including during flight. P or Pause pauses; Ctrl+Escape asks whether to end the sortie (Y/N).

The window uses square pixels and nearest-neighbour scaling. `--skip-intro` skips the opening sequence, and `--no-audio` disables playback. Run `apache --help` for other options.

Pilot profiles and preferences are saved under `$XDG_DATA_HOME/apache` or `~/.local/share/apache` on Linux, and `%LOCALAPPDATA%\apache` on Windows. `--save-dir` selects another location. Fetched game files occupy the separate `game` subdirectory on Linux, or `data` beside the Windows executable.

## Development

```sh
ctest --test-dir build --output-on-failure
```

`apache_inspect` exercises the decoded resources and simulation without opening a window. See [implementation and validation](docs/implementation.md) for the original-code comparisons and remaining work. Game data and generated captures are kept outside this repository.
