# Apache C++

A native C++26 reconstruction of Digital Integration's **Apache Longbow**, released as **Apache** in the US, from the [Lost Engines](https://github.com/Lost-Engines) reverse-engineering project.

Implementation is in progress. The reference is the January 1998 software engine. Original game data are loaded separately from a selected installation or extracted CD directory.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run the menu frontend with:

```sh
./build/apache --data-dir /path/to/extracted/cd/files
```

The menus open in a resizable window with square pixels. Click Flight, Quickstart, Preferences or Pilot’s Log; F1 opens the original help pages and Escape returns. Gameplay and editable mission/pilot state are still being connected.

`apache_inspect` checks decoded images, terrain, models and transforms. See [implementation and validation](docs/implementation.md).
