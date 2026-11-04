# Apache C++

A native C++26 reconstruction of Digital Integration's **Apache Longbow**, released as **Apache** in the US, from the [Lost Engines](https://github.com/Lost-Engines) reverse-engineering project.

Implementation is in progress. The reference is the January 1998 software engine. Original game data are loaded separately from a selected installation or extracted CD directory.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run with:

```sh
./build/apache --data-dir /path/to/extracted/cd/files
```

The game opens in a resizable window with square pixels. Flight now leads to Quickstart, training, single missions and campaigns, with mission selection, takeoff, live flight and combat, and a debrief with retry or logging. The pilot logbook supports multiple users and pilots, saved campaign progress, promotions and medals. On Linux, saves go to `$XDG_DATA_HOME/apache` or `~/.local/share/apache`; `--save-dir` selects another directory.

For a direct flight check, add `--mission 61`; `--realistic` selects the realistic flight model. Arrow keys control pitch and roll, Q/A change collective, Enter selects a weapon, Space fires, and F2/F3 select the pilot/gunner cockpit. P or Pause pauses; Ctrl+Escape ends the sortie.

This is still a development build. The complete briefing map tools and audio playback are being integrated. Graphics, flight model, difficulty and control choices can be changed in Preferences and are saved beside pilot profiles. F1 opens the menu help screen. The original game data are read without modification.

`apache_inspect` checks decoded images, terrain, models and transforms. See [implementation and validation](docs/implementation.md).
