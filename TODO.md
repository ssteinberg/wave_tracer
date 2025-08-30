
# Some TODOs

**This is not a comprehensive list, nor a plan for the research or development of future versions of wave_tracer.**\
These TODO lists are a wish list or ideas that came up. The lists are unordered, and points vary widely in scope, importance, and complexity.

-----

### Features

* beams:            rework beam extents, sourcing phase-space extents, and gracefully dealing with degenerate idealistic sensors (like point perspective).
* beams:            exact beam footprint on interaction
    * is this needed?
* emitters:         antennas
* surface bsdfs
    * Fresnel transmission for arbitrary (complex) IOR
    * all surface BSDFs should admit some transmission, IOR dictates attenuation through the medium
    * replace dielectric with smooth bsdf, handling arbitrary IOR
        * smooth bsdf: add polarizing capability
        * smooth bsdf: add birefringence
* quantities:       read power related quantities in scene files (e.g., spectrum scales)
* sensors:          sensors associated with geometry (outer lens, etc.); forward connectable
* sensors/films:    1d/3d sensors & non-imaging preview/output
* area emitters:    detector beam integration over emitter
* sensors:          additional camera models
    * thin lens
    * physical pinhole
    * lens systems?
* bsdfs:            diffraction gratings
* bvh8w:            create better trees (higher occupancy)


### Fixes

* double precision: broken tracing, why?
* double precision: make intersection tests and traversal respect double-precision


### Research

* diffractions
    * polarimetric
    * edge geometry, curved edges, electric properties
    * corners
    * better sampling
    * cross-interaction diffractions
* intersections: better clip distances and numeric tolerances
* polarimetric BSDFs
    * surface bsdfs for long-wavelength scattering (Degli & Esposti 2007 as a starting point)
    * better diffuse model?
* rough surface profiles:
    * is sampling correct?
    * sampling only of relevant surface frequencies.
* acoustics
    * integration without temporal averaging
* beams:            how to handle focusing optics well?
* medium interactions
* fluorescence and cross-spectral effects
* lens systems
* GPU
    * CUDA?
* "true" Gaussian beams: curvature, Gouy phase, anisotropic solid angle
    * is this needed?
* polarimetric sampling of interactions
    * is this needed?


### Engineering and C++ Modernization

* XML schema for scene file
    * loader/deserializer
    * serializer
    * really missing docs (priority)
* loader:           cleanup aux_tasks mess; handle dependencies well
* bitmap write2d/load2d:    handle colourspaces, linear/sRGB, etc. Currently, support is a mess (especially openexr)..
* distributed rendering
* profiling:        add memory tracking mechanism for scene elements
* profiling:        add loading time tracking for scene elements
* units:            complex units
* units:            matrices with units
* units:            resolve the current mix of dimensionless SI angles and strong angular system (``u::ang::*``)
* c++ modules


### Misc

* tests
* gui
    * simple debug views
    * allow configuring runtime defines
    * interactive scene editing
    * path tracing debug view, with visualizations of paths, beams and interactions
* extend docs
* python bindings?
