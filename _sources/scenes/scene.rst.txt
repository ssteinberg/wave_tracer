
Scene
=======================================

Scene format takes a form similar to `mitsuba <https://mitsuba-renderer.org/>`_.
A scene file describes the scene geometry, materials, and media. It also specifies emitters and sensor configuration, as well as the integrator that should be used to compute the light transport in the scene.

.. dropdown:: XML scene data sources
   :open:

   Scene files that use the XML data source look as follows:

   .. code-block:: xml

      <?xml version="1.0" encoding="utf-8"?>

      <scene version="0.1.0" >

         <default name="spe" value="1024"/>
         <default name="res" value="1280"/>
         <default name="wavelength" value="1um"/>

         <integrator type="plt_path" >
            <string name="direction" value="forward"/>
         </integrator>

         <sensor type="virtual_plane" id="coverage">
            <transform name="to_world">
               <lookat origin="0m, 0m, 0m" target="0m, 1m, 0m"/>
            </transform>
            <quantity name="extent" value="1000m, 750m" />

            <integer name="samples" value="$spe" />
            <film type="array" >
               <integer name="width" value="$res" />
               <integer name="height" value="($res*.75)" />

               <response type="monochromatic">
                  <spectrum type="discrete" wavelength="$wavelength" />
               </response>
            </film>
         </sensor>

         <emitter type="point">
            <point name="position" value="0m, 100m, 0m"/>
            <spectrum name="radiant_intensity" type="discrete" wavelength="$wavelength" value="1" />
         </emitter>

         <bsdf type="surface_spm" id="mat-itu_marble">
            <spectrum name="IOR" ITU="marble" />
            <spectrum name="extIOR" constant="1" />
         </bsdf>

         <shape type="ply" id="object">
            <path name="filename" value="[path]"/>
            <quantity name="scale" value="1cm" />
            <ref id="mat-itu_marble" name="bsdf"/>
         </shape>

      </scene>

   The scene file starts with setting default values for runtime-configurable defines.
   Then, it setups the used integrator, sensor, an emitter, and a geometric object (a shape, with its geometrical data read from a PLY file).

   The sensor is a virtual plane sensor that is used for signal coverage simulations, its extent and orientation is defined via the `to_world` transformation and `extent` quantity.
   The sensor's `film` element defines its resolution (as an array of virtual pixels) and sensitivity function.

   Note that *wave_tracer* uses physical units for all physical quantities, with one exception: scale values of `spectrum` elements are context-dependent; in the example above, the units of the `radiant_intensity` spectrum are implied to be spectral radiant intensity, i.e. *Watts per steradians per millimetre*.

   The object's material describes a smooth interface, with the exterior medium having an index-or-refraction (IOR) of 1, and internal medium with IOR spectrum defined by ITU "marble" material (defined by Recommendation ITU-R P.2040-2).

   *wave_tracer* can parse math expressions. Values that contain math expressions must appear within parentheses, for example see the "height" node of the `film` element.

   The XML schema documentation of scene elements is a work in progress.
   Sample scenes are available in the  ``/scenes`` directory.


---------------------------------------

.. toctree::
   :maxdepth: 2

   integrators/integrator
   samplers/sampler
   sensors/sensor
   emitters/emitter
   bsdfs/bsdf
   spectra/spectrum
   surface_profiles/surface_profile
   shape
   textures/texture
   transformation

   loader

---------------------------------------

.. doxygenclass:: wt::scene_t
   :members:
   :undoc-members:

---------------------------

.. doxygenclass:: wt::scene_renderer_t
   :members:
   :undoc-members:

---------------------------

.. doxygenclass:: wt::scene::scene_element_t
   :members:
   :undoc-members:

