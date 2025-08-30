
low-discrepancy Sobol sequence sampler
############################################

Use low-discrepancy Sobol sequences for sampling [Ostromoukhov2024]_.
This sampler might produce slightly lower integration variances, compared with a uniform sampler.

.. [Ostromoukhov2024] Quad-Optimized Low-Discrepancy Sequences, by Victor Ostromoukhov, Nicolas Bonneel, David Coeurjolly, Jean-Claude Iehl, 2024.
   `Link <https://github.com/liris-origami/Quad-Optimized-LDS>`_.


------------

.. doxygenclass:: wt::sampler::sobolld_t
   :members:
   :undoc-members:

