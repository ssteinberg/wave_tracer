
**************************************
Units and quantities
**************************************


Internally, **wave_tracer** uses a strongly-typed unit system for all physical quantities.
This is very useful when writing physical simulation software, and helps catch errors early.
Scene files also use units for quantities.

**wave_tracer** defines :cpp:type:`wt::f_t` as the floating-point representation type for real numbers.
:cpp:type:`wt::f_t` to the single-precision 32-bit `float`, but can be compile-time configured.

**wave_tracer** uses the `mp-units <https://github.com/mpusz/mp-units>`_ library for strongly-typed units and dimensions.
Units are defined in namespace :cpp:any:`wt::u`.
The underlying numeric representation for all these quantities is :cpp:type:`wt::f_t`.
In addition, common types for quantities, like length, wavenumber and power, are defined (see below).
For example,

.. code-block:: cpp

   length_t len = 3 * u::mm;
   area_t area = m::sqr(len);
   auto p = irradiance_t(1.1f * u::W / area);
   std::cout << std::format("{}", p) << std::endl;

defines a variable with a quantity concept of ISQ length with units of millimetre, squares it to produce area, and then constructs a quantity of radiometric irradiance.
`length_t` stores values in units of metres.
Dimensionless quantities can be cast to a unitless number :cpp:type:`wt::f_t` in two ways:

.. code-block:: cpp

   auto area_ratio = area / (.5f * u::m2);
   std::cout << std::format("{}", (f_t)area_ratio) << std::endl;
   std::cout << std::format("{}", u::to_num(area_ratio)) << std::endl;

The static cast guarantees a lossless conversion.
At times a lossless conversion is not possible, and the static cast will fail to compile:

.. code-block:: cpp

   auto area_ratio = area / m::sqr(1 * u::mm);
   std::cout << std::format("{}", (f_t)area_ratio) << std::endl;        // fails to compile!
   std::cout << std::format("{}", u::to_num(area_ratio)) << std::endl;  // compiles: forces lossy conversion

The former is not a lossless conversion because the type `area_t` uses units of metre squared; therefore, even though the type of `area_ratio` is a dimensionless quantity, its units include a static scaling constant of :math:`\tfrac{\text{m}^2}{\text{mm}^2}`, and a conversion to a unitless number cannot be done losslessly.
The explicit cast via :cpp:func:`wt::u::to_num` forces a lossy conversion.

Neither form will compile when attempting to convert from a non-dimensionless quantity.
To cast units away, different functions of the form `wt::u::to_X` are provided.
For example,

.. code-block:: cpp

   length_t len = 3 * u::mm;
   std::cout << std::format("len = {} mm", u::to_mm(1 * u::m)) << std::endl;
   std::cout << std::format("len = {} m", u::to_m(len)) << std::endl;
   std::cout << std::format("{}", u::to_Hz(1 * u::m)) << std::endl;   // fails to compile: cannot cast `mm` to `Hz`

The first cast is lossless, and simply discards the units to produce a number (of type :cpp:type:`wt::f_t`); the second cast is lossy, and first converts the representation of `len` to metres; the third cast does not compile as a conversion from millimetres to Hertz is not legal.

Quantities of same concept can be compared to each other and ordered:

.. code-block:: cpp

   static_assert(1 * u::mm < 1 * u::m);

Comparisons between quantities of different concepts will fail to compile.
Common math operations, like :cpp:func:`wt::m::sqrt`, :cpp:func:`wt::m::abs`, :cpp:func:`wt::m::inverse` are defined for quantities and behave properly.
Likewise, trigonometric functions are defined for angle quantities.
c++ concepts for different quantity concepts are also defined.
For example, the following template function accepts any angle quantities, with no runtime conversions:

.. code-block:: cpp

   auto cone_solid_angle(const Angle auto& half_opening_angle) {
      return m::four_pi * m::sqr(m::sin(half_opening_angle / 2.f));
   }

   std::cout << std::format("{}", cone_solid_angle(1 * u::ang::rad)) << std::endl;
   std::cout << std::format("{}", cone_solid_angle(m::pi/180 * u::ang::deg)) << std::endl;


General quantities
^^^^^^^^^^^^^^^^^^^^^^^^^^

The following general quantities are defined. The suffix "[unit]" indicates the units used for internal storage.

====================  ========================
length                 :cpp:type:`wt::length_t`  :math:`[\small\text{m}]`
area                   :cpp:type:`wt::area_t`  :math:`[\small\text{m}^2]`
volume                 :cpp:type:`wt::volume_t`  :math:`[\small\text{m}^3]`

length density         :cpp:type:`wt::length_density_t`  :math:`[\small\text{m}^{-1}]`
area density           :cpp:type:`wt::area_density_t`  :math:`[\small\text{m}^{-2}]`
volume density         :cpp:type:`wt::volume_density_t`  :math:`[\small\text{m}^{-3}]`

angle                  :cpp:type:`wt::angle_t`  :math:`[\small\text{radians}]`
solid angle            :cpp:type:`wt::solid_angle_t`  :math:`[\small\text{sr}]`

angle density          :cpp:type:`wt::angle_density_t`  :math:`[\small\text{radians}^{-1}]`
solid angle density    :cpp:type:`wt::solid_angle_density_t`  :math:`[\small\text{sr}^{-1}]`

wavenumber             :cpp:type:`wt::wavenumber_t`  :math:`[\small\text{mm}^{-1}]`
wavelength             :cpp:type:`wt::wavelength_t`  :math:`[\small\text{mm}]`
frequency              :cpp:type:`wt::frequency_t`  :math:`[\small\text{GHz}]`

wavenumber density     :cpp:type:`wt::wavenumber_density_t`  :math:`[\small\text{mm}]`
wavelength density     :cpp:type:`wt::wavelength_density_t`  :math:`[\small\text{mm}^{-1}]`

power                  :cpp:type:`wt::power_t`  :math:`[\text{Watt}]`
temperature            :cpp:type:`wt::temperature_t`  :math:`[\text{K}]`
====================  ========================


As long as the quantity concepts match, a quantity can be implicitly converted to another.
For example: `f_t(1) / length_t(2 * u::m)` can be implicitly captured by a :cpp:type:`wt::length_density_t`, and vice versa.

Some helper converters for quantities that quantify electrodynamic (EM) radiation frequency are defined:

====================================  ======================================
wavelength to wavenumber               :cpp:func:`wt::wavelen_to_wavenum`
wavenumber to wavelength               :cpp:func:`wt::wavenum_to_wavelen`
EM frequency to wavelength (vacuum)    :cpp:func:`wt::freq_to_wavelen`
EM frequency to wavenumber (vacuum)    :cpp:func:`wt::freq_to_wavenum`
wavenumber (vacuum) to EM frequency    :cpp:func:`wt::wavenum_to_freq`
====================================  ======================================



Radiometric quantities
^^^^^^^^^^^^^^^^^^^^^^^^^^

**wave_tracer** defines common power-derived radiometric quantities, including spectral variants.
Note that **wave_tracer** defines spectral quantities *per wavenumber*, and not per wavelength.

=================================  =====================================
radiated flux                       :cpp:type:`wt::radiant_flux_t`   :math:`[\small\text{Watt}]`
radiant intensity                   :cpp:type:`wt::radiant_intensity_t`   :math:`[\tfrac{\text{Watt}}{\text{sr}}]`
radiated irradiance                 :cpp:type:`wt::irradiance_t`   :math:`[\tfrac{\text{Watt}}{\text{m}^2}]`
radiance                            :cpp:type:`wt::radiance_t`   :math:`[\tfrac{\text{Watt}}{\text{m}^2 \cdot \text{sr}}]`

spectral radiated flux              :cpp:type:`wt::spectral_radiant_flux_t`   :math:`[\small\text{mm}\cdot\text{Watt}]`
spectral radiant intensity          :cpp:type:`wt::spectral_radiant_intensity_t`   :math:`[\tfrac{\text{mm}\cdot\text{Watt}}{\text{sr}}]`
spectral irradiance                 :cpp:type:`wt::spectral_irradiance_t`   :math:`[\tfrac{\text{mm}\cdot\text{Watt}}{\text{m}^2}]`
spectral radiance                   :cpp:type:`wt::spectral_radiance_t`   :math:`[\tfrac{\text{mm}\cdot\text{Watt}}{\text{m}^2 \cdot \text{sr}}]`
=================================  =====================================

Importance-derived quantities are also defined:

=================================  =====================================
importance flux                     :cpp:type:`wt::QE_flux_t`   :math:`[\small\text{m}^2 \cdot \text{sr}]`
importance flux per solid angle     :cpp:type:`wt::QE_area_t`     :math:`[\small\text{m}^2]`
importance flux per area            :cpp:type:`wt::QE_solid_angle_t`     :math:`[\small\text{sr}]`
importance (Quantum efficiency)     :cpp:type:`wt::QE_t`    :math:`[\small\text{dimensionless}]`
=================================  =====================================

The importance-derived quantities have a similar geometric meaning to the power-derived quantities: for example, *spectral radiated flux* and *importance flux* are both quantities that integrate emitted power or importance, respectively, over solid angle and area; *spectral irradiance* and *importance flux per area* are both differential quantities that quantify flux (power or importance, respectively) per unit area.

Non-spectral variants of importance-derived quantities above are not defined, as these are not used in practice.

Facilities that are used for light transport, like Stokes parameters vectors and beams (the primary light transport primitive), have specializations for relevant radiometric quantities.
For example, :cpp:type:`wt::spectral_radiance_stokes_t` and :cpp:type:`wt::spectral_radiance_beam_t`.



Units in scene file
^^^^^^^^^^^^^^^^^^^^^^^^^^

WIP
