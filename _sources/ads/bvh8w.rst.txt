
8-Wide BVH
=======================================

This is a fast, vectorized implementation of a SAH-based 8-wide BVH, targetting hybrid (ray- and cone-tracing) workloads.
Several optimizations that target cone tracing are implemented :cite:p:`emre2025cones`.

---------------------------

.. doxygenclass:: wt::ads::bvh8w_t
   :members:
   :undoc-members:

---------------------------

.. doxygenclass:: wt::ads::construction::bvh8w_constructor_t
   :members:
   :undoc-members:

