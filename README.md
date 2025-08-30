
<img src="icon.png" alt="icon" width="100"/>


# wave_tracer

**A wave-optical path tracer**

[![GitHub Tag](https://img.shields.io/github/v/tag/ssteinberg/wave_tracer?include_prereleases&sort=date&logo=github&logoColor=AAEEFF&label=wave_tracer&color=AAEEFF)](https://github.com/ssteinberg/wave_tracer/tags)
[![Documentation](https://github.com/ssteinberg/wave_tracer/actions/workflows/docs.yaml/badge.svg)](https://github.com/ssteinberg/wave_tracer/actions/workflows/docs.yaml)
[![(Ubuntu latest gcc) Build and Test](https://github.com/ssteinberg/wave_tracer/actions/workflows/build_test_gcc.yaml/badge.svg)](https://github.com/ssteinberg/wave_tracer/actions/workflows/build_test_gcc.yaml)
[![(Ubuntu latest clang-17) Build and Test](https://github.com/ssteinberg/wave_tracer/actions/workflows/build_test_clang17.yaml/badge.svg)](https://github.com/ssteinberg/wave_tracer/actions/workflows/build_test_clang17.yaml)
[![(Ubuntu latest clang-20) Build and Test](https://github.com/ssteinberg/wave_tracer/actions/workflows/build_test_clang20.yaml/badge.svg)](https://github.com/ssteinberg/wave_tracer/actions/workflows/build_test_clang20.yaml)



wave_tracer performs wave-optical light transport simulations.
It is designed to operate across a wide range of the electromagnetic spectrum, and targets different applications.
For example, long-wavelength simulations with cellular, WiFi or radar radiation; or optical rendering with light in the visible spectrum.
It operates by path tracing *elliptical cones*, which serve as the geometric proxies for light waves, and simulating the interaction of the underlying waves with the virtual environment.
wave_tracer is able to simulate the wave-interference phenomena that are observed when these light wave are diffracted by geometry and materials.

Currently, simulations are done on the CPU only.

**This is a work-in-progress and an early alpha release. Expect issues and an incomplete feature set.**

Some documentations are available [here](https://wavetracer.dev). Documentations are a work in progress.


\
<img src="banner.png" alt="banner" />



## Build

When cloning, include the recursive flag to fetch the submodules 

```
git clone --recursive https://github.com/ssteinberg/wave_tracer
```

Submodules can be updated, after cloning the base repository, using

```
git submodule update --init --recursive
```

Compile using CMake:
```
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build .
```

A recent c++20 compatible compiler is required. See [**BUILD.md**](BUILD.md) for dependencies and additional compilation information.

This repository uses [Git LFS](https://git-lfs.github.com/) to manage large files such as scene assets and datasets. On an Ubuntu-based system, git LFS can be installed via `apt install git-lfs`.
If Git LFS was installed after cloning the repository, the LFS-tracked files can be manually pulled via `git pull`.




## Usage

The rendering subcommands are ``render`` and ``renderui``:
* ``render`` uses a command line interface. Rendering preview can be dynamically viewed during rendering with [tev](https://github.com/Tom94/tev), using the ``--tev`` flag.
* ``renderui`` uses a built-in GUI, which provides simple rendering preview, with polarimetric support, as well as displays some scene data and runtime statistics. Only available when compiled with the ```BUILD_GUI``` CMake flag.

See 
```
./wave_tracer --help
```
for additional command line options.


### Examples

1.
    Wave-optical rendering with optical sensors (visible spectrum):
    ```
    ./wave_tracer renderui ../scenes/cornell-box/box.xml -D res=1440,spp=1024
    ```
    ```
    ./wave_tracer renderui ../scenes/bidir_room/room.xml -D res=1920,spp=1024
    ```
    Scenes may use *runtime defines* (``-D``) to setup some properties, for example, the sensor's resolution and samples-per-element count are given by defines above.

1.
    Long-wavelength 10GHz signal coverage simulation in a city:
    ```
    ./wave_tracer renderui ../scenes/sionna_etoile/etoile.xml -D res=720,spp=1024 -D wavelength=10GHz
    ```
    The above visualizes the irradiance that falls upon a virtual plane placed at street level, that acts as a virtual sensor.
    A tonemapping operator converts the output to Decibels and applies a colourmap.

    Classical (ray-based) path tracing can be forced using the ``--ray-tracing`` flag.
    For example, we can simulate the signal coverage in the same `etoile` scene, but with ray tracing only (not accounting for diffractions), and compare the difference in output:
    ```
    ./wave_tracer renderui ../scenes/sionna_etoile/etoile.xml -D res=720,spp=1024 -D wavelength=10GHz --ray-tracing
    ```

1. 
    The double-slit diffraction pattern can be rendered using
    ```
    ./wave_tracer renderui ../scenes/diffraction_simple/double_slits.xml -D res=1440,spp=1024 -D pattern=true
    ```
    the above renders the diffraction pattern itself using a virtual plane sensor.
    Alternatively, we may render the scene from the perspective of an idealized perspective sensor, and see the pattern reflected off the wall:
    ```
    ./wave_tracer renderui ../scenes/diffraction_simple/double_slits.xml -D res=1440,spp=1024 -D pattern=false
    ```
    Visualization is, by default, in colour-coded Decibels.
    Runtime defines can be used to control the slit distance and size (e.g., ``-D W=.65,Wslit=.25``).

    Scenes may setup multiple sensors and emitters operating at different regimes of the electromagnetic spectrum.\
    For example, an overview of the above double slit scene may be rendered using a simple optical sensor:
    ```
    ./wave_tracer renderui ../scenes/diffraction_simple/double_slits.xml -D res=1440,spp=256 -D optical_overview=true
    ```
    Note that the scene remains unchanged, runtime defines (``-D``) toggle which sensor is used. Emitters are importance sampled based on their spectral power overlap with the sensor's sensitivity spectrum; when there is no overlap, emitters are not simulated.

Additional sample scenes are available at `/scenes`.



## Citing

If you find this work useful for your research, please cite
```
@article{Steinberg_wt_2025,
    title={Wave Tracing: Generalizing The Path Integral To Wave Optics}, 
    author={Shlomi Steinberg and Matt Pharr},
    month={aug},
    year={2025},
    eprint={2508.17386},
    archivePrefix={arXiv},
    primaryClass={physics.optics},
	journal={arXiv},
    url={https://ssteinberg.xyz/2025/08/28/generalizing_the_path_integral/}, 
}
```


----

Released under the **CC Attribution-NonCommercial 4.0 International** license

@Copyright *Shlomi Steinberg*
