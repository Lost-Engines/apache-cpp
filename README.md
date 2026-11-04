# Apache C++

A native C++26 reconstruction of Digital Integration's **Apache Longbow**, released as **Apache** in the US, from the [Lost Engines](https://github.com/Lost-Engines) reverse-engineering project.

Implementation is in progress. The reference is the January 1998 software engine. Original game data are loaded separately from a selected installation or extracted CD directory.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The initial `apache_inspect` utility checks decoded images and terrain against the original analysis. The playable frontend is being connected next. See [implementation and validation](docs/implementation.md).
