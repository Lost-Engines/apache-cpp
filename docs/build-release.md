# Building and packaging

Use CMake 3.30 or newer and a compiler accepting C++26. GCC 14 or newer is used for release builds. Linux needs OpenGL and the X11/Wayland development libraries used by GLFW. macOS builds use GCC for C++ and Apple's Objective-C compiler for Cocoa. Windows builds use MSYS2 UCRT64 GCC.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j3
ctest --test-dir build --output-on-failure
```

GLFW and miniaudio sources are fetched at pinned, checksum-verified revisions. No game assets are fetched by CMake or the tests. `scripts/fetch-assets.sh` and `scripts/fetch-assets.bat` acquire the original game data separately.

For portable packages, configure with `-DAPACHE_STATIC_RUNTIME=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_DISABLE_FIND_PACKAGE_glfw3=TRUE`. This bundles GLFW and statically links the compiler runtimes. Linux still uses system graphics, windowing and C libraries.

Install into an empty staging directory, then package it:

```sh
cmake --install build --prefix "$PWD/package/apache" --component Runtime
cmake -DPACKAGE_ROOT="$PWD/package" -DOUTPUT_DIR="$PWD/dist" -DPLATFORM=linux-x86_64 -P cmake/package.cmake
```

The other platform names are `windows-x86_64` and `macos-arm64`. The install step copies the executable, setup helper, documentation and dependency licences from an explicit list. Packaging rejects unexpected files and symbolic links. Game resources, original executables, saves, inspector tools, captures and test caches are excluded.

The extensive original-code comparisons live in the sibling analysis workspace. CTest runs the self-contained resource, scene, profile, packet and localhost transport tests. Headless window checks use a private X server; they never send input to the desktop.
