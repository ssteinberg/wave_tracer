cmake_minimum_required(VERSION 3.26.0)
cmake_policy(VERSION 3.26.0)

include(CMakePrintHelpers)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# read build type
string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
# and set build target to the common debug or release
if(build_type STREQUAL debug OR build_type STREQUAL debugog)
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_BUILD_TYPE "Release")
    message("DEBUG build")
elseif(build_type STREQUAL release OR build_type STREQUAL profile)
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_BUILD_TYPE "Release")
    message("RELEASE build")
else()
    message(FATAL_ERROR "Unrecognized CMAKE_BUILT_TYPE.")
endif()
cmake_print_variables(build_type)


# -- vcpkg --
# Prefer passing -DCMAKE_TOOLCHAIN_FILE=... on the command line.
# If not provided, fall back to $VCPKG_ROOT.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain file")
endif()
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message("Using vcpkg")
endif()



# -- project --
project(wave_tracer VERSION 0.1.0 LANGUAGES CXX)

# sources
set(SRC

    src/main.cpp
    src/version.cpp

    src/static_tests.cpp

    src/ads/bvh_constructor.cpp
    src/ads/bvh8w.cpp
    src/ads/bvh8w_constructor.cpp

    src/bitmap/srgb_lut.cpp
    src/bitmap/texture2d_loader.cpp
    src/bitmap/load2d.cpp
    src/bitmap/write2d.cpp

    src/beam/beam.cpp

    src/bsdf/bsdf_loader.cpp
    src/bsdf/composite.cpp
    src/bsdf/dielectric.cpp
    src/bsdf/diffuse.cpp
    src/bsdf/mask.cpp
    src/bsdf/normalmap.cpp
    src/bsdf/scale.cpp
    src/bsdf/surface_spm.cpp
    src/bsdf/two_sided.cpp

    src/emitter/emitter_loader.cpp
    src/emitter/area.cpp
    src/emitter/directional.cpp
    src/emitter/point.cpp
    src/emitter/spot.cpp

    src/interaction/intersection.cpp
    src/interaction/fsd/free_space_diffraction.cpp
    src/interaction/fsd/fraunhofer/free_space_diffraction.cpp
    src/interaction/fsd/fraunhofer/fsd_lut.cpp
    src/interaction/fsd/fraunhofer/fsd_sampler.cpp
    src/interaction/surface_profile/surface_profile_loader.cpp
    src/interaction/surface_profile/dirac.cpp
    src/interaction/surface_profile/fractal.cpp
    src/interaction/surface_profile/gaussian.cpp

    src/integrator/integrator_loader.cpp
    src/integrator/plt_bdpt.cpp
    src/integrator/plt_path.cpp

    src/mesh/mesh.cpp
    src/mesh/cube.cpp
    src/mesh/prism.cpp
    src/mesh/cylinder.cpp
    src/mesh/icosahedron.cpp
    src/mesh/lens.cpp
    src/mesh/rectangle.cpp
    src/mesh/sphere.cpp
    src/mesh/obj_loader.cpp
    src/mesh/ply_loader.cpp

    src/sampler/sampler_loader.cpp
    src/sampler/sobolld.cpp
    src/sampler/uniform.cpp

    src/sensor/sensor_loader.cpp
    src/sensor/perspective.cpp
    src/sensor/virtual_plane_sensor.cpp

    src/sensor/response/response_loader.cpp
    src/sensor/response/monochromatic.cpp
    src/sensor/response/multichannel.cpp
    src/sensor/response/RGB.cpp
    src/sensor/response/XYZ.cpp
    src/sensor/response/tonemap.cpp
    src/sensor/mask.cpp

    src/scene/loader/loader.cpp
    src/scene/loader/xml/loader.cpp
    src/scene/render.cpp
    src/scene/scene_build_sensor_sampling_data.cpp
    src/scene/scene.cpp
    src/scene/scene_element.cpp
    src/scene/scene_sensor.cpp
    src/scene/shape.cpp

    src/spectrum/spectrum_loader.cpp
    src/spectrum/analytic.cpp
    src/spectrum/binned.cpp
    src/spectrum/blackbody.cpp
    src/spectrum/composite.cpp
    src/spectrum/uniform.cpp
    src/spectrum/discrete.cpp
    src/spectrum/function.cpp
    src/spectrum/gaussian.cpp
    src/spectrum/piecewise_linear.cpp
    src/spectrum/rgb.cpp
    src/spectrum/complex_analytic.cpp
    src/spectrum/complex_uniform.cpp
    src/spectrum/complex_container.cpp
    src/spectrum/util/spectrum_from_db.cpp
    src/spectrum/util/spectrum_from_ITU.cpp

    src/texture/texture_loader.cpp
    src/texture/bitmap.cpp
    src/texture/checkerboard.cpp
    src/texture/constant.cpp
    src/texture/function.cpp
    src/texture/mix.cpp
    src/texture/scale.cpp
    src/texture/transform.cpp
    src/texture/complex_constant.cpp
    src/texture/complex_container.cpp

    src/math/elliptic_cone.cpp
    src/math/gaussian2d.cpp
    src/math/product_distribution.cpp
    src/math/transform_loader.cpp

    src/util/font_renderer.cpp
    src/util/logger.cpp
    src/util/math_expression.cpp
    src/util/preview_tev.cpp
    src/util/tpool.cpp
    
    src/util/statistics_collector/stat_collector_registry.cpp
    src/util/statistics_collector/stat_histogram.cpp
)


# -- CMake flags --
option(BUILD_GUI "Add graphical user interface support" ON)
option(DBL_PRECISION "Use 64-bit double precision floating points" OFF)
option(SIMD_AVX "Enable SIMD support: for single-precision requires AVX2, for double-precision AVX512f" ON)


# -- executable and resources --
if(APPLE)
    set(RESOURCES
        icon.icns
    )

    # Mark files to land in MyApp.app/Contents/Resources
    foreach(res ${RESOURCES})
        set_source_files_properties(${res} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    endforeach()

    add_executable(wave_tracer MACOSX_BUNDLE
        ${SRC}
        ${RESOURCES}
    )

    set_target_properties(wave_tracer PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.ssteinberg.wave_tracer"
        MACOSX_BUNDLE_BUNDLE_NAME "wave_tracer"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        # MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Info.plist.in"
    )
else()
    add_executable(wave_tracer ${SRC})
endif()


# -- compilation options --

# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COLOR_DIAGNOSTICS ON)

add_compile_definitions(WTVERSION="${PROJECT_VERSION}")

if(DBL_PRECISION)
    add_compile_definitions(_DBL_SUPPORT)
    add_compile_definitions(_FLOAT_TYPE=double)
    message("Using 64-bit double-precision floating points")
else()
    add_compile_definitions(_NO_DBL_SUPPORT)
    add_compile_definitions(_FLOAT_TYPE=float)
    message("Using 32-bit single-precision floating points")
endif(DBL_PRECISION)

# SIMD
if(SIMD_AVX)
    message("SIMD turned on: using AVX")

    add_compile_definitions(SIMD_AVX)
    add_compile_definitions(__AVX2__)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(wave_tracer PRIVATE
            /arch:AVX2
        )
    else()
        target_compile_options(wave_tracer PRIVATE 
            -mavx -mavx2
        )
    endif()

    # double precision requires AVX512f/dq support
    if(DBL_PRECISION)
        message("Compiling with double-precision and AVX: ensure target has AVX512f/dq support")

        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            target_compile_options(wave_tracer PRIVATE
                /arch:AVX512
            )
        else()
            target_compile_options(wave_tracer PRIVATE 
                -mavx512f -mavx512dq
            )
        endif()    
    endif(DBL_PRECISION)
endif(SIMD_AVX)

if(WIN32)
	add_compile_definitions(_windows)
elseif(APPLE)
	add_compile_definitions(_apple)
else()
	add_compile_definitions(_linux)
endif()


# -- compiler flags --

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(wave_tracer PRIVATE
	-march=native
        -pipe 
        -funroll-loops
        -fno-rounding-math -fno-math-errno
        -fno-signaling-nans
        -Wmaybe-uninitialized
        -Wunused-but-set-variable
        -Wpedantic
        -Wdisabled-optimization
        -Wno-psabi
    )
    target_link_options(wave_tracer PRIVATE
        -Wno-psabi
    )
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(wave_tracer PRIVATE 
	-march=native
        -pipe 
        -funroll-loops
        -fno-rounding-math -fno-math-errno
        -Wunused-but-set-variable
        -Wno-psabi
    )
    target_link_options(wave_tracer PRIVATE
        -Wno-psabi
    )
endif()

# sanitize in debug
if(build_type STREQUAL debug)
    add_compile_definitions(DEBUG)

    set_property(TARGET wave_tracer PROPERTY INTERPROCEDURAL_OPTIMIZATION False)

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(wave_tracer PRIVATE 
            /Odi
            /Zi
            /fsanitize=address
        )
        target_link_options(wave_tracer PRIVATE
            /DEBUG
        )
    else()
        target_compile_options(wave_tracer PRIVATE 
            -g -O0
            -fsanitize=undefined -fsanitize=address
        )
        target_link_options(wave_tracer PRIVATE
            -g -O0
            -fsanitize=undefined -fsanitize=address
        )
    endif()
endif()

# Og debug
if(build_type STREQUAL debugog)
    add_compile_definitions(DEBUG)

    set_property(TARGET wave_tracer PROPERTY INTERPROCEDURAL_OPTIMIZATION False)

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(wave_tracer PRIVATE 
            /Odi /GS
            /Zi
        )
        target_link_options(wave_tracer PRIVATE
            /DEBUG
        )
    else()
        target_compile_options(wave_tracer PRIVATE 
            -g -Og
        )
        target_link_options(wave_tracer PRIVATE
            -g -Og
        )
    endif()
endif()

# lto in release
if(build_type STREQUAL release)
    add_compile_definitions(NDEBUG)
    add_compile_definitions(RELEASE)

    set_property(TARGET wave_tracer PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(wave_tracer PRIVATE 
            /O2 /Oy
        )
    else()
        target_compile_options(wave_tracer PRIVATE 
            -O3 -fomit-frame-pointer
            -ftree-vectorize -flto=auto
        )
        target_link_options(wave_tracer PRIVATE
            -O3
            -flto=auto
        )
    endif()
endif()

# profile (symbols + optimizations + additional perf stats) builds
if(build_type STREQUAL profile)
    add_compile_definitions(NDEBUG)
    add_compile_definitions(PROFILE)

    add_compile_definitions(_additional_ads_stats)
    add_compile_definitions(_additional_plt_stats)
    add_compile_definitions(_additional_bitmap_stats)
    add_compile_definitions(_additional_scene_stats)

    set_property(TARGET wave_tracer PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(wave_tracer PRIVATE 
            /O2 /Oy
        )
    else()
        target_compile_options(wave_tracer PRIVATE 
            -O3 -g -fomit-frame-pointer
            -ftree-vectorize -flto=auto
        )
        target_link_options(wave_tracer PRIVATE
            -O3 -g
            -flto=auto
        )
    endif()
endif()


# -- external dependencies --
find_package(PNG)
find_package(libdeflate CONFIG REQUIRED)
find_package(OpenEXR CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)

find_package(TBB CONFIG REQUIRED)

find_package(yaml-cpp CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)


target_include_directories(wave_tracer PRIVATE 
    include

    ${OPENEXR_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(wave_tracer PRIVATE
    stdc++fs atomic

    PNG::PNG
    $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_shared>,libdeflate::libdeflate_shared,libdeflate::libdeflate_static>
    OpenEXR::OpenEXR
    Freetype::Freetype

    TBB::tbb
    
    yaml-cpp::yaml-cpp
    pugixml::pugixml
)

if(BUILD_GUI)
    find_package(SDL3 CONFIG REQUIRED)

    target_include_directories(wave_tracer PRIVATE 
        ${SDL3_INCLUDE_DIRS}
    )
    target_link_libraries(wave_tracer PRIVATE
        SDL3::SDL3
    )
endif(BUILD_GUI)


# tinyexpr++ defines
add_compile_definitions(TE_NO_BOOKKEEPING)
if(NOT DBL_PRECISION)
    add_compile_definitions(TE_FLOAT)
endif()


# GUI
if(BUILD_GUI)
    add_compile_definitions(GUI)

    add_compile_definitions(IMGUI_DISABLE_DEBUG_TOOLS)
    add_compile_definitions(IMGUI_DISABLE_DEMO_WINDOWS)
    add_compile_definitions(IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
    add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    add_compile_definitions(IMGUI_USER_CONFIG=\"wt/util/gui/imgui/imgui_config.hpp\")

    find_package(GLEW CONFIG REQUIRED)

    target_sources(wave_tracer PRIVATE
        src/util/gui/fonts.cpp
        src/util/gui/gui.cpp
        src/util/gui/init.cpp
        src/util/gui/scene_info.cpp
        src/util/gui/perf_stat.cpp

        src/util/gui/imgui_tex_inspect/imgui_tex_inspect.cpp
        src/util/gui/imgui_tex_inspect/imgui_tex_inspect_opengl.cpp
    )

    target_include_directories(wave_tracer PRIVATE 
        ${GLEW_INCLUDE_DIRS}
    )

    target_link_libraries(wave_tracer PUBLIC
        GL
        GLEW::GLEW
    )
endif(BUILD_GUI)


# deps
add_subdirectory(deps)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

