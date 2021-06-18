# Chia CLI plotter - plotter with automatic ID generation and bucket sorting in RAM
* Built-in plot ID generation for provided farmer(-F param) and public key(-p)
  ** ID generator is prebuilt in DLL so no need for rebuilding difficult to build crypto stuff
* Automatically build multiple plots with -n param (just like with full Chia suite)
* Sort directly in RAM
  ** Currently makes sorting 30% faster
  ** still uses SSD for caching tables but all sorting buckets are now kept in memory
  ** No RAMdisk or any other abstractions just using big buffers and bitsets hence 180GB RAM usage and 1GB stack size YOLO
  ** To make this viable better CPU parallelization is still required
* Only native - no need for Python to build or run
* Dynamic library IdgenLib.dll is currently built for Windows so will need some work to be playable on other platforms
* Not sure if the last plots were valid, some stuff might need to be commented out to allow generating valid plots

# Usage - prebuilt
## Warning! Requires at least 256GB RAM for k32 plots
* Plotter.exe create -k 28 -F farmer_key -p pool_key
* prebuilt version is in BIN folder
* (optional) after building your own Plotter.exe make sure to add IdgenLib.dll to it's folder

### Windows development
#### Common steps
* Install VSCode and C++ extension
* The extensions should automatically detect GCC, MSBUILD or Clang - if installed
#### MSBUILD
* Install Visual Studio v16(not 2016 but 2019 v16 preview) with Windows SDK and C++ stuff
* Optionally add CLANG for another compilation alternative
#### GCC
* Install MSys2 and follow this manual to install mingw64 https://www.msys2.org/
* Add mingw64 /bin folder to path - default is C:\msys2\mingw64\bin
* Install CMake and add to path
* configure mingw64 path in .vscode/tasks.json and gdc in .vscode/launch.json

### Building BLS-Signature
* Good luck in Windows, hence the pre-built DLL included in /lib/include/IdgenLib.dll that must be placed next to Plotter.exe(or added to PATH)


# ORIGINAL README

# Chia Proof of Space
![Build](https://github.com/Chia-Network/chiapos/workflows/Build/badge.svg)
![PyPI](https://img.shields.io/pypi/v/chiapos?logo=pypi)
![PyPI - Format](https://img.shields.io/pypi/format/chiapos?logo=pypi)
![GitHub](https://img.shields.io/github/license/Chia-Network/chiapos?logo=Github)

[![Total alerts](https://img.shields.io/lgtm/alerts/g/Chia-Network/chiapos.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Chia-Network/chiapos/alerts/)
[![Language grade: Python](https://img.shields.io/lgtm/grade/python/g/Chia-Network/chiapos.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Chia-Network/chiapos/context:python)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/Chia-Network/chiapos.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Chia-Network/chiapos/context:cpp)

Chia's proof of space is written in C++. Includes a plotter, prover, and
verifier. It exclusively runs on 64 bit architectures. Read the
[Proof of Space document](https://www.chia.net/assets/Chia_Proof_of_Space_Construction_v1.1.pdf) to
learn about what proof of space is and how it works.

## C++ Usage Instructions

### Compile

```bash
# Requires cmake 3.14+

mkdir -p build && cd build
cmake ../
cmake --build . -- -j 6
```

### Run tests

```bash
./RunTests
```

### CLI usage

```bash
./ProofOfSpace -k 25 -f "plot.dat" -m "0x1234" create
./ProofOfSpace -k 25 -f "final-plot.dat" -m "0x4567" -t TMPDIR -2 SECOND_TMPDIR create
./ProofOfSpace -f "plot.dat" prove <32 byte hex challenge>
./ProofOfSpace -k 25 verify <hex proof> <32 byte hex challenge>
./ProofOfSpace -f "plot.dat" check <iterations>
```

### Benchmark

```bash
time ./ProofOfSpace -k 25 create
```


### Hellman Attacks usage

There is an experimental implementation which implements some of the Hellman
Attacks that can provide significant space savings for the final file.


```bash
./HellmanAttacks -k 18 -f "plot.dat" -m "0x1234" create
./HellmanAttacks -f "plot.dat" check <iterations>
```

## Python

Finally, python bindings are provided in the python-bindings directory.

### Install

```bash
python3 -m venv .venv
. .venv/bin/activate
pip3 install .
```

### Run python tests

Testings uses pytest. Linting uses flake8 and mypy.

```bash
py.test ./tests -s -v
```

## ci Building
The primary build process for this repository is to use GitHub Actions to
build binary wheels for MacOS, Linux (x64 and aarch64), and Windows and publish
them with a source wheel on PyPi. See `.github/workflows/build.yml`. CMake uses
[FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
to download [pybind11](https://github.com/pybind/pybind11). Building is then
managed by [cibuildwheel](https://github.com/joerick/cibuildwheel). Further
installation is then available via `pip install chiapos` e.g.

## Contributing and workflow
Contributions are welcome and more details are available in chia-blockchain's
[CONTRIBUTING.md](https://github.com/Chia-Network/chia-blockchain/blob/main/CONTRIBUTING.md).

The main branch is usually the currently released latest version on PyPI.
Note that at times chiapos will be ahead of the release version that
chia-blockchain requires in it's main/release version in preparation for a
new chia-blockchain release. Please branch or fork main and then create a
pull request to the main branch. Linear merging is enforced on main and
merging requires a completed review. PRs will kick off a GitHub actions ci build
and analysis of chiapos at
[lgtm.com](https://lgtm.com/projects/g/Chia-Network/chiapos/?mode=list). Please
make sure your build is passing and that it does not increase alerts at lgtm.
