
**************************************
Theoretical foundations
**************************************

This chapter discusses some of the theoretical foundations and science behind **wave_tracer**, and makes a formal connection between wave optics and classical (ray-optical) light transport.

Throughout this text, we use the notation ":math:`[\text{units}]`" to indicate the units of physical quantities.

We will use script capital letters (for example, ":math:`\mathcal{W}`") for *Wigner distribution functions* :cite:p:`testorf2010phase`, which are joint space-frequency distributions that describe the spatial, spectral and angular distribution of light.
Wigner distribution functions are real-valued but not strict distributions: they may take negative values and may be not normalized.

We use the capital letter "S" to indicate dimensionless quantities that relate to the sensitivity of a sensor, and the capital letter "W" to indicate emitted *spectral radiance*.



=====================================
Sensing
=====================================

When radiation impinges upon an electromagnetic sensor---like the biological photoreceptors in the human Eye, a CMOS array in a camera, or an antenna---an electrical current is excited, producing an observed signal.
The power :math:`I` :math:`[\text{Watt}]` of that electric signal, as *sensed* by a sensor (or a sensor's element), is

.. math::
   I =
      \iint
         \mathrm{d}\vec{r}
         \mathrm{d}\vec{k}\;
            \mathcal{S}\left( \vec{r},\vec{k} \right)
            \mathcal{L}\left( \vec{r},\vec{k} \right)
   ~,
   :label: sensing_wdfs

where :math:`\vec{r}\in\mathbb{R}^3` :math:`[\text{mm}^{3}]` is a spatial position and :math:`\vec{k}\in\mathbb{R}^3` :math:`[\text{m}^{-3}]` is the *wavevector*.
The Wigner distribution function :math:`\mathcal{S}` :math:`[\text{dimensionless}]` represents the sensor's sensitivity distribution.
:math:`\mathcal{S}` is a property of the sensor, and is non-zero only at points :math:`\vec{r}` that lie upon the sensor.
:math:`\mathcal{L}` :math:`[\text{Watt}]` is also a Wigner distribution function (though of different units) that represent the distribution of radiated power in the entire scene.

The wavevector quantifies the radiation's direction of propagation and wavelength.
Its magnitude is known as the *wavenumber* :math:`k=\lvert\vec{k}\rvert=\frac{2\pi}{\lambda}` :math:`[\mathrm{mm}^{-1}]`, where :math:`\lambda` :math:`[\mathrm{mm}]` is the wavelength.
The normalized wavevector :math:`\hat{d}=\vec{k}/k` is the direction of the radiation's propagation.

The differential wavevector can be expressed in terms of its differential wavenumber and direction as

.. math::
   \mathrm{d}\vec{k}
      = k^2\mathrm{d}k\;\mathrm{d}\hat{d}
      = \left(\frac{2\pi}{\lambda}\right)^2 2\pi\frac{\mathrm{d}\lambda}{\lambda^2} \mathrm{d}\hat{d}
   \quad\quad
   [\text{mm}^{-3}]

(up to an irrelevant sign).
Then, we rewrite Equation :eq:`sensing_wdfs` as:

.. math::
   I =
      \int_0^\infty
         \frac{\mathrm{d}\lambda}{\lambda^4}
         \left( 2\pi \right)^3
      \int
         \mathrm{d}\vec{r}
      \int
         \mathrm{d}\hat{d}\;
            \mathcal{S}\left( \vec{r},\hat{d},\lambda \right)
            \mathcal{L}\left( \vec{r},\hat{d},\lambda \right)
   ~.
   :label: sensing_wdfs_sensor_area_direction

The arguments to :math:`\mathcal{S},\mathcal{L}` now explicitly include :math:`\hat{d}` and :math:`\lambda`.
This is a notational change only: recall that :math:`\vec{k} = \frac{2\pi}{\lambda} \hat{d}`.

The reformulation in Equation :eq:`sensing_wdfs_sensor_area_direction` above useful for a couple of reasons:

1. It makes the wavelength :math:`\lambda` and direction of propagation :math:`\hat{d}` explicit. This will be useful when we numerically integrate the sensing formula above.
2. And, it allows us to recognize important radiometric quantities that are involved in light transport. We do so next.


.. _radiometry:

Making a connection to radiometry
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We perform a brief dimensional analysis of Equation :eq:`sensing_wdfs_sensor_area_direction`.
Note the units of the differentials:
:math:`\mathrm{d}\lambda` :math:`[\text{mm}]`,
:math:`\mathrm{d}\vec{r}` :math:`[\text{m}^2]`, and
:math:`\mathrm{d}\hat{d}` :math:`[\text{sr}]`.
:math:`\mathcal{S}` :math:`[\text{dimensionless}]` remains the sensor's sensitivity distribution.
Then, as :math:`I` :math:`[\text{Watt}]` is the power sensed by the sensor,
the quantity :math:`\tfrac{(2\pi)^3}{\lambda^4}\mathcal{L}` must have units of power per wavelength per surface area per solid angle,
which we recognize as the radiometric *spectral radiance* :math:`[\tfrac{\text{Watt}}{\text{mm}\cdot\text{m}^2\cdot\text{sr}}]`.

It then makes sense to define

.. math::
   \hat{\mathcal{L}} \equiv
      \frac{(2\pi)^3}{\lambda^4}
      \mathcal{L}
   ~,
   :label: L

which is the distribution of spectral radiance in the scene.
This important quantity is discussed further in the next section.

The dimensionless sensor sensitivity distribution :math:`\mathcal{S}` is the *Quantum efficiency* of the sensor (the power ratio between the produced electric signal to the incident photon flux).
In path tracing literature :math:`\mathcal{S}` is sometimes known as the (spectral) *importance*.
For every spatial position :math:`\vec{r}` that lies on the sensor, :math:`\mathcal{S}` quantifies the sensor response to radiation that arrives from direction :math:`\hat{d}` with wavelength :math:`\lambda`.

Note that our connection to radiometry is purely based on dimensional analysis, no restrictions on :math:`\mathcal{L}` (or :math:`\hat{\mathcal{L}}`) or :math:`\mathcal{S}` have been placed: these remain general Wigner distribution functions.

.. dropdown:: Units and quantities in **wave_tracer**

   **wave_tracer** uses a strongly-typed system for units and dimensions.
   See :doc:`units`.



=========================================
Light Transport
=========================================

We now recast the above as a light transport problem.

Let :math:`\mathcal{W}( \vec{r},\hat{d},\lambda )` defines the spatial, angular and spectral distribution of radiated spectral radiance of an emitter:
That is, for every spatial position :math:`\vec{r}` that lies upon an emitter and wavelength :math:`\lambda`, :math:`\mathcal{W}( \vec{r},\hat{d},\lambda )` is the spectral radiance emitted by the emitter in direction :math:`\hat{d}`.

We define the *light transport operator*, denoted :math:`\mathbf{\Pi}`, such that the (equilibrium) distribution of spectral radiance throughout the scene (defined in Equation :eq:`L`) fulfils the equality :math:`\hat{\mathcal{L}} = \mathbf{\Pi}\mathcal{W}`.

Both :math:`\mathcal{W}` and :math:`\hat{\mathcal{L}}` are Wigner distribution functions with units of spectral radiance, however note the difference between the two:
:math:`\mathcal{W}` is an *un-transported* quantity---it is non-zero only at spatial positions that lie upon the emitter, and depends on the emitter's properties only.
Just like the sensor's sensitivity distribution, we assume that :math:`\mathcal{W}` is given to us as input.
On the other hand, :math:`\hat{\mathcal{L}}` is the distribution of (equilibrium) spectral radiance throughout, after propagation and interaction with the scene.

.. dropdown:: Transient (non-equilibrium) light transport

   Some applications of interest are concerned with transient (non-equilibrium) effects, in which case the light transport operator becomes time-gated, and the sensing formula :eq:`sensing_wdfs_sensor_area_direction` needs to change slightly and integrate over a finite time duration.

   As of version 0.1, **wave_tracer** does not support transient light transport, and, for simplicity, we ignore transient phenomena in this text.


Solving for the transported quantity :math:`\hat{\mathcal{L}}` directly is only practical in the most simple of cases.
Instead, we would like to numerically integrate an integral equation.
Applying the light transport operator to Equation :eq:`sensing_wdfs_sensor_area_direction` yields

.. math::
   I =
      \int_0^\infty
         \mathrm{d}\lambda
      \int
         \mathrm{d}\vec{r}
      \int
         \mathrm{d}\hat{d}\;
            \mathcal{S}\left( \vec{r},\hat{d},\lambda \right)
            \left[ \mathbf{\Pi}\mathcal{W} \vphantom{\hat{d}} \right]
               \left( \vec{r},\hat{d},\lambda \right)
   ~.
   :label: lt_eq

*Light transport algorithms* take an input a representation of a scene (its geometry, materials, etc.), the sensor's sensitivity distribution :math:`\mathcal{S}` and emitter's emission distributions :math:`\mathcal{W}`, and solve for the observed sensor's response :math:`I`.
An integral part of a light transport algorithm, and often the most computationally intensive, is numerically evaluating the light transport operator.


.. dropdown:: Sensing and emission

   In **wave_tracer**, :doc:`scenes/sensors/sensors/sensor` and their :doc:`scenes/sensors/response_functions/response_function` implement sensor models and quantify sensors' sensitivity functions :math:`\mathcal{S}`.
   :doc:`Emitters <scenes/emitters/emitter>` model emitters and quantify their emission spectra.
   Both sensor and emitter objects provide sampling facilities in-order to emit importance or spectral radiance beams; as well as to connect incident beams and integrate for the sensed response power :math:`I`.


Time-reversed dynamics
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For practical reasons, it often becomes useful in light transport to discuss *time-reversed dynamics*, formalized by the light transport operator :math:`\mathbf{\Pi}^{\text{tr}}` ("tr" stands for "time-reversed").
Equation :eq:`lt_eq` may then be recast in an equivalent manner as

.. math::
   I =
      \int_0^\infty
         \mathrm{d}\lambda
      \int
         \mathrm{d}\vec{r}
      \int
         \mathrm{d}\hat{d}\;
            \left[ \mathbf{\Pi}^{\text{tr}}\mathcal{S} \vphantom{\hat{d}} \right]
               \left( \vec{r},\hat{d},\lambda \right)
            \mathcal{W}\left( \vec{r},\hat{d},\lambda \right)
   ~.
   :label: lt_eq_backward

.. dropdown:: Proof

   To prove the equivalence between Equations :eq:`lt_eq` and :eq:`lt_eq_backward`,
   we first express the general light transport operator in terms of a phase-space kernel:

   .. math::
      \mathbf{\Pi}\mathcal{W}
         \left( \vec{r}_o,\vec{k}_o \right)
         =
         \iint
            \mathrm{d}\vec{r}_i
            \mathrm{d}\vec{k}_i
            K\left( \vec{r}_i,\vec{k}_i, \vec{r}_o,\vec{k}_o \right)
            \mathcal{W}\left( \vec{r}_i,\vec{k}_i \right)
      ~,

   where :math:`K` is a kernel function that fully defines the operator :math:`\mathbf{\Pi}`, and :math:`\mathcal{W}` is an arbitrary Wigner distribution function.
   Any operator can be expressed using a kernel function :cite:p:`testorf2010phase`.

   We define the time-reversed operator :math:`\mathbf{\Pi}^{\text{tr}}` via the Kernel
   :math:`K^{\text{tr}}( \vec{r}_i,\vec{k}_i, \vec{r}_o,\vec{k}_o ) \equiv K( \vec{r}_o,\vec{k}_o, \vec{r}_i,\vec{k}_i )`,
   i.e. the same kernel but with its argument pairs (input and output) swapped.
   Then, the inner functional product becomes:

   .. math::
      \langle
         \mathcal{S},
         \mathbf{\Pi} \mathcal{W}
      \rangle
      =&
         \iint
            \mathrm{d}\vec{r}
            \mathrm{d}\vec{k}
            \mathcal{S}
               \left( \vec{r},\vec{k} \right)
            \left[\mathbf{\Pi}\mathcal{W}\right]
               \left( \vec{r},\vec{k} \right)
      \\
      =&
         \iint
            \mathrm{d}\vec{r}
            \mathrm{d}\vec{k}
            \mathcal{S}
               \left( \vec{r},\vec{k} \right)
            \iint
               \mathrm{d}\vec{r}^\prime
               \mathrm{d}\vec{k}^\prime
               K\left( \vec{r}^\prime,\vec{k}^\prime, \vec{r},\vec{k} \right)
               \mathcal{W}\left( \vec{r}^\prime,\vec{k}^\prime \right)
      \\
      =&
         \iint
            \mathrm{d}\vec{r}^\prime
            \mathrm{d}\vec{k}^\prime
            \mathcal{W}\left( \vec{r}^\prime,\vec{k}^\prime \right)
         \iint
            \mathrm{d}\vec{r}
            \mathrm{d}\vec{k}
               K^{\text{tr}}\left( \vec{r},\vec{k}, \vec{r}^\prime,\vec{k}^\prime \right)
               \mathcal{S}
                  \left( \vec{r},\vec{k} \right)
      \\
      =&
      \langle
         \mathbf{\Pi}^{\text{tr}} \mathcal{S},
         \mathcal{W}
      \rangle
      ~,

   which completes the proof.

Equations :eq:`lt_eq` and :eq:`lt_eq_backward` are mathematically equivalent, but describe different light transport semantics:
the former transports emitted light *forward* in time, and then integrates the transported distribution over the support of the sensor's sensitivity distribution; the latter propagates the sensor's sensitivity distribution, i.e. the spectral importance, *backward* (as in: under time-reversed dynamics) and integrates over the emitters' emission distributions.

.. dropdown:: Backward vs forward light transport

   For practical reasons, backward (time-reversed) transport, where importance is propagated from the sensor, is the far more common approach to take for classical path tracers.
   On the other hand, for long-wavelength wave simulations, where many surfaces are highly specular, forward transport becomes an attractive alternative.

   **wave_tracer**'s ':doc:`scenes/integrators/plt_path` can do either forward or backward transport, with one being more effective than the other depending on the setting and application.

Without formalising, we note that bi-directional strategies are also possible, where both distributions are propagated and integrated over each other's support.


Monte Carlo integration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To numerically evaluate Equation :eq:`lt_eq` above, we Monte Carlo integrate it by taking :math:`N` samples:

.. math::
   I \approx
      \frac{1}{N}\sum_{n=1}^N
         \frac{1}{p(\lambda_n)}
         \frac{1}{p(\vec{r}_n)}
         \frac{1}{p(\hat{d}_n)}
         \mathcal{S}\left( \vec{r}_n,\hat{d}_n,\lambda_n \right)
         \left[ \mathbf{\Pi}\mathcal{W} \vphantom{\hat{d}} \right]
            \left( \vec{r}_n,\hat{d}_n,\lambda_n \right)
   ~.
   :label: mc_lt_eq

:math:`\vec{r}_n`, :math:`\hat{d}_n`, :math:`\lambda_n` are the sampled sensor's surface spatial position, direction and wavelength, respectively.
The probability densities,
:math:`p(\lambda_n)` :math:`[\text{mm}^{-1}]`,
:math:`p(\vec{r}_n)` :math:`[\text{m}^{-2}]`, and
:math:`p(\hat{d}_n)` :math:`[\text{sr}^{-1}]`,
are the wavelength, spatial position and direction sampling densities, respectively.
Numeric integration of the equivalent Equation :eq:`lt_eq_backward` is done in an identical manner.

As a shorthand, we define
:math:`p_n = p(\lambda_n) \cdot p(\vec{r}_n) \cdot p(\hat{d}_n)`
to be the overall sampling density, and
:math:`f_n = [ \mathcal{S} \cdot \mathbf{\Pi}\mathcal{W} ]( \vec{r}_n,\hat{d}_n,\lambda_n )`
to be the sample contribution.
The Monte Carlo estimator then becomes :math:`\tfrac{1}{N}\sum_n \tfrac{f_n}{p_n}`.

Monte Carlo integration is often a good fit for light transport simulations.
Light transport algorithms then compute the Monte Carlo contributions by simulating the light transport operator for each sample.
We discuss how to do so next.

.. dropdown:: Spectral sampling

   To importance sample a wavelength :math:`\lambda_n` that is used to evaluate a Monte Carlo sample contribution, **wave_tracer** constructs spectral distribution proportional to the product of the sensitivity and emission spectra for all sensor-emitter pairs.
   That is, let :math:`s(k)` and :math:`w(k)` be the marginalized sensitivity and emission spectrum for a sensor and an emitter, respectively.
   Note that **wave_tracer** stores spectra as function of wavenumber, and not wavelength.
   Then, the spectral distribution

   .. math::
      \Lambda\left( k \right)
      \equiv
         \frac{s(k)w(k)}
              {\int \mathrm{d}k\; s(k)w(k) }

   is used as the sampling distribution for this particular sensor-emitter pair.

   The normalizing constant above, :math:`\int \mathrm{d}k\; s(k)w(k)` :math:`[\text{Watt}]`, quantifies the overlap between the sensitivity and emission spectra, and is used for emitter sampling strategies.
   Spectral multiple importance sampling is also employed.

   **wave_tracer** provides extensive facilities to define and work with different spectra, see :doc:`scenes/spectra/spectrum`.



=========================================
Wave-Optical Path Tracing
=========================================

Before discussing our wave-optical light transport algorithm, it is worthwhile to briefly summarise the driving assumptions made by classical light transport algorithms.
Classical path tracing will not be discussed in detail, the reader is referred to the excellent `PBR book <https://pbr-book.org/>`_ for reference.


In the classical (ray-optical) limit
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the classical limit (i.e., the short-wavelength limit), where physics are governed by ray optics, the propagation of light follows, point-wise, geometric rays.
In this limit, every Wigner distribution function---including every sensitivity distribution :math:`\mathcal{S}` and every emission distribution :math:`\mathcal{W}`---can be understood as a collection of independent particles that follow ray trajectories.

Under the context of ray optics, the *light transport primitive* is an idealized ray, and its Wigner distribution takes the form:

.. math::
   \mathcal{R}_{\vec{o},\hat{p}}
      \left( \vec{r},\hat{d} \right)
   =
      \delta\left( \vec{r}-\vec{o} \vphantom{\hat{d}} \right)
      \delta\left( \hat{d}-\hat{p} \right)
   ~,
   :label: wdf_ray

where :math:`\delta` is the Dirac delta distribution.
:math:`\mathcal{R}` describes a singular particle with origin :math:`\vec{o}` and direction of propagation :math:`\hat{p}`---i.e., a classical ray.
We may understand a sensitivity distribution as a superposition of ray distributions :math:`\mathcal{R}`, as follows:

.. math::
   \mathcal{S}
   \left( \vec{r},\hat{d},\lambda \right)
   =
      \sum_j
      S_j
      \mathcal{R}_{\vec{o}_j,\hat{p}_j}
         \left( \vec{r},\hat{d},\lambda \right)
   ~,
   :label: wdf_decomposition_rays

where :math:`S_j \geq 0` are the (dimensionless) importances of each ray, and all the rays are assumed to be pairwise mutually independent.
A decomposition into independent rays, each with positive spectral radiance, for an emission distribution takes an identical form, except the coefficient :math:`S_j` would now take units of power.

Light transport simulations take advantage of the simplicity of ray-optical dynamics:
Instead of propagating an entire distribution, i.e. :math:`\mathbf{\Pi}\mathcal{W}` or :math:`\mathbf{\Pi}^{\text{tr}}\mathcal{S}`, it sufficient to propagate a single ray from the sampled position-direction pair on the sensor or emitter---a significantly simpler task, in general.

In summary: to evaluate a sample contribution :math:`f_n`, classical path tracing algorithms trace the trajectory of a single ray :math:`\mathcal{R}` that would arrive from the sample direction :math:`\hat{d}_n` to the sample position :math:`\vec{r}_n` on the sensor (or emitter).
When the ray impinges upon matter, path tracing Monte Carlo integrates a scattered ray from the interaction point (for example, a reflection from a surface), and proceed to ray trace recursively until the ray in connected to an emitter (or sensor).

**Assumptions**
To enable the above, the classical limit implies a couple of guiding assumptions that are instrumental to classical path tracing:

1. *incoherence*: meaning, the independence between distinct rays in the decomposition in Equation :eq:`wdf_decomposition_rays`, as well as the non-negativity of the importance :math:`S_j` that is carried by each ray.
2. *locality*: that is, at any time instant a ray exists in a singular position and propagates into a singular direction, as formalised by its distribution :math:`\mathcal{R}`.


Generalizing to wave optics
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The pair of assumptions above---instrumental to classical light transport tools---can only co-exist in the classical (ray-optical) limit :cite:p:`Steinberg_rtplt_2024`.
In general, Wigner distribution functions are in-fact not strict distributions, but *pseudo-distributions* that take negative values (as a matter of fact, all physical Wigner distribution functions, with the exception of only one class of functions, take negative values).
These negative values should be understood as a consequence of wave interference: the underlying fields are not independent particles, but admit some degree of *coherence* and interfere with each other.

.. dropdown:: What physical phenomena may be reproduced via classical light transport?

   It is interesting to consider the limits of the classical approximation of independent rays, and which optical phenomena may be reproduced within that approximation.
   The curious reader is referred to :cite:t:`Steinberg_wt_2025`, where this question is formally investigated.

To reproduce diffractive phenomena beyond the capabilities of classical frameworks, one of these two assumptions must be dropped.
Then, we are faced with a couple of options:

1. Develop a **local but not incoherent** framework: that is, trace rays (locality) but allow interference between rays (non-incoherence).
2. Develop an **incoherent but not local** framework: the classical ray is replaced with a light transport primitive that is no longer perfectly local---it has a non-Dirac distribution in position or direction of propagation.

.. dropdown:: Generalizing the classical path-space integral

   The classical path-space integral :cite:p:`VeachThesis` is a powerful general formulation of classical light transport.
   A generalization of the path-space integral to each of the approaches outlined above is possible, see :cite:t:`Steinberg_wt_2025`.

The former approach might seem attractive, but the loss of incoherence is limiting:
the interference between all rays make sampling difficult: in a sense, all sampling now becomes a *global problem*.
Practical Monte Carlo integration heavily relies on good sampling for convergence, and non-incoherent frameworks do not scale well to complex light transport.
The second approach sacrifices the convenience and niceties of purely-local rays and ray tracing, but we are rewarded with a framework that, if designed appropriately, can be incoherent in terms of its light transport primitives.
This incoherence would enable applying the same powerful sampling tools that classical path tracing leverage.



The light transport primitive: a Gaussian beam
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The key idea is then to forgo the classical ray, and replace it with an alternative light transport primitive.

We employ *Gaussian beams* as our light transport primitive.
It has been show that sensitivity and emission distributions, :math:`\mathcal{S}` and :math:`\mathcal{W}`, can be decomposed into *coherent modes* of such Gaussian beams, in a rather general setting :cite:p:`Steinberg_rtplt_2024`.
That is, we may write the sensitivity distribution of a sensor as a (countable) sum of such beams:

.. math::
   \mathcal{S}
   \left( \vec{r},\hat{d},\lambda \right)
   =
      \sum_j
      S_j
      \mathcal{G}_{\vec{r}_n, \vec{k}_n}
         \left( \vec{r},\hat{d},\lambda \right)
   ~,
   :label: gaussian_beams

where :math:`\mathcal{G}` is the Wigner distribution function of a Gaussian beam, and :math:`\vec{r}_n,\vec{k}_n` are the mean spatial position and wavevector, respectively, of each Gaussian beam.
:math:`S_j` are the importance carried by each beam.
Each beam is also parameterized by a few additional parameters, like its covariances, but these are omitted here in text for brevity.
A similar decomposition follows for :math:`\mathcal{S}`.
Please see :cite:t:`Steinberg_wt_2025,Steinberg_rtplt_2024` for more information on the decomposition process and the sampling of Gaussian beams.

Note that a Gaussian beam :math:`\mathcal{G}` generalizes the singular ray :math:`\mathcal{R}` (at the limit where the Gaussian beam's covariance is zero).
However, while the ray is not *physical*, a Gaussian beam is, therefore the simulation of the propagation of that beam and its interaction with matter is able to reproduce wave-optical effects (in their generality, up to what is reproducible by the Wigner formalism).

.. dropdown:: The *physicality* of a Wigner distribution function

   The Wigner distribution function of some wavefunction :math:`\psi` is given by

   .. math::
      \mathcal{W}
      \left( \vec{r},\vec{k} \right)
      =
         \left( \frac{1}{2\pi} \right)^3
         \int
            \mathrm{d}\vec{r}^\prime
            \psi\left( \vec{r} - \frac{1}{2}\vec{r}^\prime \right)
            \psi^\star\left( \vec{r} + \frac{1}{2}\vec{r}^\prime \right)
            \mathrm{e}^{ \mathrm{i} \vec{k}\cdot\vec{r}^\prime }
      ~,

   where :math:`\star` denotes the complex conjugate.
   A Wigner distribution function :math:`\mathcal{W}` is said to be *Wigner-realizable* if there exists a wavefunction :math:`\psi` that produces :math:`\mathcal{W}` via the formula above.

   The distribution of a ray, :math:`\mathcal{R}`, is not Wigner-realizable---there does not exist a wavefunction that gives rise to such a distribution---and hence it is not physical.
   On the other hand, a Gaussian beam (given some constraints on its covariances) is physical: it arises from a well-defined wavefunction, therefore electromagnetic interactions with matter are well defined.

Gaussian beams admit simple temporal dynamics: free-space propagation involves simple linear transformations of their means and covariances (see :cite:t:`Steinberg_rtplt_2024`).
Interaction of a Gaussian beam with matter---while still complicated to simulate in general---is much more tractable compared to the interaction of an arbitrary WDF with matter.

In addition, a Gaussian beam is a *weakly-local* light transport primitive: while the (perfectly-local) ray exists at any time instant at a singular position and propagates into a singular direction, the (weakly-local) beam exists at a small region and propagates into a small solid angle.
It can be shown, that an *elliptical cone* serves as a geometric envelope for the Gaussian beam :cite:p:`Steinberg_wt_2025`: meaning that propagating the beam, and deducing which part of the scene it interacts with, reduces to the geometric problem of tracing its elliptical conic envelope.

.. dropdown:: The elliptical conic envelope of a Gaussian beam

   .. figure:: /_static/cone.svg
      :figwidth: 18em
      :alt: Elliptical cone

      An illustration of an elliptical cone.

   The geometric envelope of a Gaussian beam is an elliptical cone.
   In **wave_tracer**, the geometry of an elliptical cone is defined via :cpp:class:`wt::elliptic_cone_t`.
   In addition, intersection queries for elliptical cones are provided in namespace :cpp:any:`wt::intersect`.

.. dropdown:: Beams and polarization in **wave_tracer**

   Beams (:cpp:class:`wt::beam_t`) provide facilities to quantify the Gaussian beam's wavefront---needed for light-matter interactions---and its elliptical conic envelope---needed for propagation---as well as computation of interaction regions and beam transformations.

   Beams also carry the polarimetric radiometric or importance quantities associated with the beam (:cpp:class:`wt::beam::beam_radiometric_data_t`).
   Different specializations are provided for forward and time-reversed beams:
   For example, :cpp:type:`wt::spectral_radiance_beam_t` is a beam designed for forward transport, it carries Stokes parameters vector with units of spectral radiance.
   The beam :cpp:type:`wt::importance_flux_beam_t` carries a Mueller operator with units of importance flux :math:`[\small\text{m}^2 \cdot \text{sr}]` (:cpp:type:`wt::QE_flux_t`).

   When radiometric (forward) and importance (time-reversed) beams connect, as part of their integration Mueller-Stokes calculus is applied in-order to calculate their polarimetric throughout.
   See :cite:t:`korotkova2017random` or :cite:t:`Steinberg_practical_plt_2022` for more information on (generalized) Mueller-Stokes calculus.



Path tracing with beams
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When computing a Monte Carlo sample contribution, we sample an importance Gaussian beam from the sensor, :math:`\mathcal{G}_{ \vec{r}^\prime,\vec{k}^\prime }`.
This is possible due to the decomposition into Gaussian beams outlined in Equation :eq:`gaussian_beams`.
Then, employing the time-reversed formulation (Equation :eq:`lt_eq_backward`) the sample contribution becomes

.. math::
   f_n =
         \left[
            S_n
            \mathbf{\Pi}^{\text{tr}}
            \mathcal{G}_{ \vec{r}^\prime_n,\vec{k}^\prime_n }
         \vphantom{\hat{d}} \right]
            \left( \vec{r}_n,\hat{d}_n,\lambda_n \right)
         \mathcal{W}\left( \vec{r}_n,\hat{d}_n,\lambda_n \right)
   ~,
   :label: mc_contribution_gaussian_backward

where :math:`S_n` :math:`[\text{dimensionless}]` is the importance carried by the sampled beam.
Alternatively, we may sample a spectral radiance Gaussian beam from the emitter, and the contribution becomes

.. math::
   f_n =
         \mathcal{S}\left( \vec{r}_n,\hat{d}_n,\lambda_n \right)
         \left[
            W_n
            \mathbf{\Pi}
            \mathcal{G}_{ \vec{r}^\prime_n,\vec{k}^\prime_n }
         \vphantom{\hat{d}} \right]
            \left( \vec{r}_n,\hat{d}_n,\lambda_n \right)
   ~,
   :label: mc_contribution_gaussian_forward

where :math:`W_n` :math:`[\tfrac{\text{Watt}}{\text{mm}\cdot\text{m}^2\cdot\text{sr}}]` is the spectral radiance carried by the sampled beam.

Equations :eq:`mc_contribution_gaussian_forward` and :eq:`mc_contribution_gaussian_backward` formalise the single-sample Monte Carlo contribution for light transport with Gaussian beams.
Our approach for solving for the transported beam :math:`\mathbf{\Pi}\mathcal{G}_{ \vec{r}^\prime_n,\vec{k}^\prime_n }` (or the time-reversed variant) is very similar to classical path tracing:

1. Propagate the Gaussian beam, this is done via tracing its geometric envelope---an elliptical cone---until the envelope intersects (or partially intersects) geometry.
2. Simulate the light-matter interaction in the interaction region (the intersection region) of the incident Gaussian beam with the matter within the region.
3. Monte Carlo sample a scattered Gaussian beam from the interaction.
4. Repeat steps 1-3 recursively, until the beam connects to an emitter or sensor.

It is easy to extend the above to include volumetric interactions.

.. dropdown:: Accelerating data structures for elliptical cone traversal

   Similar to ray traversal, cone traversal also benefits greatly from an accelerating data structure---a tree data structure used to optimize traversal in a scene.
   **wave_tracer** implements dedicated :doc:`ads/ads` that target hybrid (ray and cone traversal) workloads, as some operations still benefit from ray queries.

The above outline of a light transport algorithm is a path tracing algorithm: intuitively, instead of decomposition emission and importance distributions of emitters and sensors into rays, and sampling the paths (i.e. *path trace*) taken by these rays, we decompose these distributions into Gaussian beams, and path trace these beams.
This is a generalization of classical path tracing---which is restricted to the classical (ray-optical) limit---to a more general formulation that is compatible with wave optics.
This is the algorithm used in **wave_tracer**.

.. dropdown:: Light transport integrators

   In **wave_tracer** :doc:`scenes/integrators/integrator` implement different variants of the wave-optical path tracing algorithm above.
   Integrators are responsible for the sampling of beams from emitters and sensors, sampling interactions, implementing and sampling connection strategies, as well as accumulating :math:`f_n/p_n` for the Monte Carlo estimator.





(*this document may be extended in future releases*)
