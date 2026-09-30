# Building & Running TurnoV00m

## Tools

Install [MSYS2](https://www.msys2.org/). Then, from the **MSYS2 UCRT64** shell:

```bash
pacman -S mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-make \
          mingw-w64-ucrt-x86_64-SDL2 \
          mingw-w64-ucrt-x86_64-SDL2_mixer \
          mingw-w64-ucrt-x86_64-SDL2_net \
          mingw-w64-ucrt-x86_64-libpng
```

Verify: `cmake --version` and `gcc --version` both print a version.

## Build

From the repo root, in the **MSYS2 UCRT64** shell:

```bash
cmake -S . -B build -G "Ninja"
cmake --build build -j
```

The executable is produced at `build/src/crispy-doom.exe`.

> Re-run the first command (`cmake -S . -B build ...`) after editing `CMakeLists.txt`
> or adding source files. For a clean rebuild: `rm -rf build` first.

## Run

The WAD files live elsewhere, so pass the IWAD path explicitly with `-iwad`:

```bash
./build/src/crispy-doom.exe -iwad /c/path/to/doom1.wad
```

Any IWAD works: `doom1.wad` (shareware), `doom.wad`, `doom2.wad`, or `freedoom1.wad`.

## Package distributable

To build the distributable you can just run the make-dist.ps1 script. This will create a dist folder containing the final TurnoV00m directory with the self contained .exe and files. This directory can then be compressed to a zip file and distributed to any pc or totem and is ready to run.

```
./make-dist.ps1
```
