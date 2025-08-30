
## How to build

Build using CMake:
```
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build .
```

Requires a recent compiler with decent [**c++20**](https://en.cppreference.com/w/cpp/compiler_support.html#C.2B.2B20_features) support.

Tested on (using an Ubuntu LTS 24.04 system)
 * **gcc 13.3** / **14.2**
 * **clang 17.0** / **20.1**

For targets that do not support the required AVX instruction sets (for example, ARM-based Macs and MacBooks), AVX vectorization can be manually disabled via the CMake option:
```
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -DSIMD_AVX:BOOL=OFF
```

GUI support requires a working OpenGL environment.

To build this project and its external dependencies, several common system dependencies are required.
On Ubuntu 24.04 the necessary packages can be pulled via:
```
sudo apt install git-lfs build-essential pkg-config libxmu-dev libxi-dev libgl-dev libltdl-dev libx11-dev libxft-dev libxext-dev libwayland-dev libxkbcommon-dev libegl1-mesa-dev libibus-1.0-dev
```



## CMake options

```BUILD_GUI```: (default `ON`) flag to build the graphical user interface (GUI).

```DBL_PRECISION```: (default `OFF`) use double-precision 64-bit floating point arithmetics.

```SIMD_AVX```: (default `ON`) enable SIMD using AVX instruction sets. Requires ISA support for the following instruction sets:
* **AVX2** --- when compiling with single-precision floating-point arithmetics
* **AVX512f** and **AVX512dq** --- when compiling with double-precision support (```DBL_PRECISION``` set to `ON`)



### Build type configuration

`CMAKE_BUILD_TYPE` may be set to one of the following:

* ```debug```: Debug build. **Symbols** and *undefined* and *address* **sanitizers**. Very slow.
* ```debugOg```: Debug build. **Symbols** and **-Og optimizations**. No sanitizer.
* ```profile```: Profile build. **Symbols** and **-O3 and LTO optimizations**. Collects additional runtime performance statistics (like ray/cone tracing performance, integrator statistics, etc.)
* ```release```: No symbols. **-O3 and LTO optimizations**. Reduced runtime error checking.



## Dependencies

**wave_tracer** uses the following external dependencies:
 * png
 * libdeflate
 * openexr
 * pugixml
 * freetype2
 * tbb
 * yaml-cpp

The external dependencies can be installed using the system package manager.

Alternatively, dependencies management can be done automatically using [`vcpkg`](https://github.com/microsoft/vcpkg).
If `vcpkg` has not been previously installed, clone and install as follows:
```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
cd ..
```
With slight modifications to the above on non-Unix systems (for example, change "./vcpkg" to ".\vcpkg.exe" in a Windows PowerShell).

Once `vcpkg` integration has been installed, export the environment variable `VCPKG_ROOT` that specifies where `vcpkg` was installed or define the `CMAKE_TOOLCHAIN_FILE` CMake variable, and build can continue using CMake as above. For example on a Unix-based system:
```
VCPKG_ROOT=</path/to/vcpkg> cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build .
```
Or, in a Windows PowerShell:
```
$env:VCPKG_ROOT='</path/to/vcpkg>'
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build . --config Release
```
`vcpkg` will pull and install the external dependencies.
Compilation of the external dependencies might require several system packages, see [above](#how-to-build).

This repository uses [Git LFS](https://git-lfs.github.com/) to manage large files such as scene assets and datasets. On an Ubuntu-based system, git LFS can be installed via `apt install git-lfs`.
If Git LFS was installed after cloning the repository, the LFS-tracked files can be manually pulled via `git pull`.


### git submodules

When cloning, include the recursive flag to fetch the submodules 

```
git clone --recursive https://github.com/ssteinberg/wave_tracer
```
Submodules can be updated, after cloning the base repository, using

```
git submodule update --init --recursive
```

This project uses the following submodules:

**math**
 * [glm](https://github.com/g-truc/glm)
 * [gcem](https://github.com/kthohr/gcem)
 * [libcerf](https://jugit.fz-juelich.de/mlz/libcerf)

**networking**
 * [sockpp](https://github.com/fpagliughi/sockpp)

**image support**
 * [avir](https://github.com/avaneev/avir)

**BVH construction**
 * [tinybvh](https://github.com/jbikker/tinybvh)

**3D mesh loaders**
 * [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)
 * [miniply](https://github.com/vilya/miniply)

**units system**
 * [mp-units](https://github.com/mpusz/mp-units)

**utilities**
 * [CLI11](https://github.com/CLIUtils/CLI11)
 * [magic_enum](https://github.com/Neargye/magic_enum)
 * [tinycolormap](https://github.com/yuki-koyama/tinycolormap)
 * [tinyexpr-plusplus](https://github.com/Blake-Madden/tinyexpr-plusplus)

**c++ backward compatibility**
 * [function2](https://github.com/Naios/function2)

**GUI dependencies**
 * [imgui](https://github.com/ocornut/imgui)
 * [implot](https://github.com/epezent/implot)
 * [imgui_toggle](https://github.com/cmdwtf/imgui_toggle)



## Documentations

Documentations sources are available in `/docs`.
Compiling the documentations require *doxygen* and a working python3 environment with *sphinx* and several extensions: *breath*, *exhale*, *bibtex*, *mathjax*, *design* and *furo*.
On an Ubuntu-based system with a python3 environment, the above can be installed via
```
apt install doxygen
pip install sphinx exhale breath sphinx-mathjax-offline sphinxcontrib.bibtex sphinx-design furo
```

To build the documentations:

```
cmake -Hdocs -Bdocsbuild
cmake --build docsbuild --target docs Sphinx
```
the documentations will be compiled into `/docsbuild/sphinx`.
