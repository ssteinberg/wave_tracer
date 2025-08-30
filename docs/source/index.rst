
wave_tracer
=======================================

| A wave-optical path tracer
| For optical rendering and long-wavelength simulations

**wave_tracer** is an academic project that implements research in wave-optical light transport simulations, and serves to conduct further research.
It targets different applications: for example, long-wavelength simulations with cellular, WiFi or radar radiation; or optical rendering with light in the visible spectrum.
It operates by path tracing elliptical cones, which act as the geometric proxies for light waves, and simulating the interaction of the underlying waves with the virtual environment.
**wave_tracer** simulates the wave-interference phenomena that arise when these light wave are diffracted by geometry and materials.

The system is designed to operate across different ranges of the electromagnetic spectrum.
**wave_tracer** provides extensive facilities for representing and working with different types of spectra; for quantifying the emission spectra of emitters, sensitivity spectra of sensors, and the wavelength-dependent properties of materials.

The system modular design and used scene representation are heavily inspired by `mitsuba <https://mitsuba-renderer.org/>`_.
**wave_tracer** uses a strongly typed system for physical :doc:`units and dimensions <units>`, both for scene files and internally.

**Current release is an early alpha release with a limited set of implemented features.**
Support for additional features, emitter, sensor and interaction models will be provided in future releases.


.. image:: _static/banner.png
   :width: 90%
   :align: center
   :alt: banner


.. toctree::
   :maxdepth: 2
   :hidden:

   scenes/scene
   ads/ads
   cli
   units
   theory
   bib
   Full API <api/library_root>


------

By `Shlomi Steinberg <https://ssteinberg.xyz>`_

Released under the **CC Attribution-NonCommercial 4.0 International** license

