
Command line interface
###########################

**global options**

-h, --help                  Prints help message and exits

subcommands
***********************

:ref:`version`
        Print version
:ref:`render`
        Render a scene
:ref:`renderui`
        Render a scene with a GUI


.. _version:

version
^^^^^^^^^^^^^^^^^^^^^^^

Prints build version of wave_tracer:

``./wave_tracer version``

This subcommand takes no additional arguments and has no options.


.. _render:

render
^^^^^^^^^^^^^^^^^^^^^^^

Renders a scene file. Usage:

``./wave_tracer render <scene_file>``

:positionals:

`scene_file PATH`
            scene file to render

:options:

-o, --output PATH             rendered results output directory
--scenedir PATH           path for scene resources loading (defaults to scene file
                              directory)
-p, --threads UINT            number of parallel threads to use (defaults to hardware
                              concurrency)
-D, --define <NAME=VALUE>       define variables (used as "$variable" in the scene file)
--ray-tracing                 forces ray tracing only
--mesh_scale TEXT
                              default scale to apply to imported positions of external meshes;
                              can be overridden per shape in the scene file.
                              default: ``1m``

*verbosity*

-q, --quiet
                              Excludes: --verbose --verbosity
                              suppresses debug/info output (log level = quiet)
-v, --verbose
                              Excludes: --quiet --verbosity
                              additional informational output (log level = info)
--verbosity <quiet/important/normal/info/debug>
                              sets verbosity level of standard output logging
--no-progress             suppresses progress bars

*preview interface*

--tev
                              connect to a tev instance to display rendering preview (hostname:port)
                              default: ``127.0.0.1:14158``

*renderer fine tuning*

--block_size UINT
                              dimension size of a rendered image block
                              default: ``24``
--block_samples UINT
                              number of samples-per-pixel for a single rendered image block
                              default: ``8``

*run-time performance statistics*

--print-stats, --no-print-stats
                          toggles printing performance statistics to stdout on exit,
                          default: ``TRUE`` unless ``--quiet`` is set
--write-stats, --no-write-stats
                          write performance statistics to a CSV file on exit

*logging*

--filelog, --no-filelog
                          toggles logging to a file
--filelog_verbosity <quiet/important/normal/info/debug>
                          sets verbosity level of file logging

*misc*

--watermark, --no-watermark
                          disables watermarking the rendered output image


.. _renderui:

renderui
^^^^^^^^^^^^^^^^^^^^^^^

Renders a scene file using a graphical user interface. Usage:

``./wave_tracer renderui <scene_file>``

:positionals:

`scene_file PATH`
            scene file to render

:options:

-o, --output PATH             rendered results output directory
--scenedir PATH           path for scene resources loading (defaults to scene file
                              directory)
-p, --threads UINT            number of parallel threads to use (defaults to hardware
                              concurrency)
-D, --define <NAME=VALUE>       define variables (used as "$variable" in the scene file)
--ray-tracing                 forces ray tracing only
--mesh_scale TEXT
                              default scale to apply to imported positions of external meshes;
                              can be overridden per shape in the scene file.
                              default: ``1m``

*renderer fine tuning*

--block_size UINT
                              dimension size of a rendered image block
                              default: ``24``
--block_samples UINT
                              number of samples-per-pixel for a single rendered image block
                              default: ``8``

*run-time performance statistics*

--print-stats, --no-print-stats
                          toggles printing performance statistics to stdout on exit,
                          default: ``TRUE`` unless ``--quiet`` is set
--write-stats, --no-write-stats
                          write performance statistics to a CSV file on exit

*logging*

--filelog, --no-filelog
                          toggles logging to a file
--filelog_verbosity <quiet/important/normal/info/debug>
                          sets verbosity level of file logging

*misc*

--watermark, --no-watermark
                          disables watermarking the rendered output image

