
PLT uni-directional path tracer
###################################

Spectral uni-directional physical light transport (PLT) path tracer :cite:p:`Steinberg_wt_2025,Steinberg_rtplt_2024,Steinberg_practical_plt_2022`.
Support either forward (from emitter) or backward (from sensor) path tracing.
Implements UTD-based edge diffractions :cite:p:`Steinberg_wt_2025`, see :doc:`../bsdfs/fsd/UTD`.

Current implementation and provides partial support for multiple UTD diffractions across a path.


---------------------------

.. doxygenclass:: wt::integrator::plt_path_t
   :members:
   :undoc-members:

