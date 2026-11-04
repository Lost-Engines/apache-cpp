# Implementation and validation

The reconstruction follows the January 1998 Windows software engine using data shared by the September 1998 and October 2000 European discs. It reads the original resource formats directly. There is no CPU emulator, translated executable or dependency on the analysis workspace in the game.

The sibling analysis project supplies original-code reference runs and independent numerical tests. C++ comparisons are added as each subsystem is implemented. Passing original-against-original repeatability tests is not counted as C++ acceptance.

Current implementation:

- Bounded binary readers and case-insensitive resource lookup.
- Packed and planar IFF images with ByteRun1 compression and indexed palettes.
- Terrain heights and scenery placements.
- Model geometry, attachments, bounded drawing programs and authored animation branches.
- Original rounded sine/cosine, arcsine and arctangent lookup, with Q31 heading/pitch/roll matrices and angle recovery.
- Software line, polygon, mesh, translucent, heat-haze, affine-textured and Gouraud-shaded rasterization.
- Complete model drawing with authored attachments, fixed-point grid coordinates, lighting, animation branches, level-of-detail chains and faction colour/texture swaps.
- Original palette, intensity ramps and glass remaps for all four warzones.
- Terrain grid projection, 7,200-entry scene queue, texture selection and fog shading tables.
- Seven terrain drawing paths, including 64-triangle subdivision and perspective correction in 16-pixel spans.
- Complete terrain and feature scene assembly, visible-sector selection, and the original ground/sky horizon.
- Bounded feature-record decoding, distant feature variants and 2D world-window clipping.
- Perspective projection and five-plane clipping with interpolated texture coordinates and lighting.
- Preference parsing, conversion and serialization.
- GLFW window presentation with integer scaling, square pixels and correctly transformed mouse coordinates.
- Original menu font, picture/button/text/slider/sprite drawing and 636 recovered widget definitions.
- Initial menu navigation and a headless frame capture option.

Validation completed:

- All 242 IFF images match the independently verified decoders: 57,344,504 pixels.
- Four terrain grids match 262,144 additional interpolated-height queries and every scenery placement count.
- Ten initialized menu frames match the original renderer exactly: 3,072,000 RGB pixels. These cover the main menu, highlighted buttons, Quickstart, exit, flight, warzones, preferences, top ten, pilot log and help.
- All 1,314 model records and 15,768 drawing cases match, including 899,317 ordered primitive/attachment events.
- 22,744 transformation cases match all 204,696 original matrix coefficients exactly.
- 14,386 raster cases match 44,193,792 original indexed pixels, covering all edge octants and the supported solid, mesh, texture, masking and lighting combinations.
- 6,605 clipping/projection cases match 256,528 output coordinate and attribute fields. Another 10,000 line-clipping cases match.
- 4,000 matrix/grid cases match 48,000 matrix coefficients and 520,000 rotated grid entries.
- Palette setup matches 266,240 bytes across the four warzones.
- 7,483 complete model frames match the original exactly across six camera/state/quality variations per model. Another 401 cases reach missing normals or out-of-texture samples in the original; those unsafe reads are excluded. The C++ renderer keeps the authored colour for a missing normal and skips a sample beyond its texture allocation.
- The whole model catalogue and the boundary tests run under AddressSanitizer and UndefinedBehaviorSanitizer without errors.
- 160 terrain samples match 201,160 vertices and 1,408,120 projected fields.
- 400 scene queues match 47,128 accepted submissions, including equal-depth terrain ordering and the original capacity limit.
- Four fog intensity tables match all 262,144 bytes; 2,048 terrain texture selections and 44,856 shading decisions also match.
- 7,000 complete terrain polygon frames match the original, including clipping, both diagonals, fog transitions and all seven rendering paths. Another 1,400 comparisons pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- All 7,124 feature records match 156,159 vertices and 73,884 faces. Rendering matches 13,997 original feature frames, including distant variants. A further 330 comparisons pass under sanitizers.
- 6,324 inverse-trigonometry cases match all 25,296 angle fields.
- 480 viewpoints match 10,488 sector-bound, origin and visibility fields.
- 6,245 2D clipping cases match 123,036 projected coordinate/intensity fields.
- 1,524 horizon images match all 206,233,600 pixels across camera angles and weather conditions.
- 192 assembled landscape images match all pixels across the four warzones and terrain-quality levels. These include normal features, terrain sorting and the horizon; clouds, celestial bodies, scenery and moving objects are the next integration steps. Another 80 landscape and 424 horizon comparisons pass under sanitizers.
- 2,485 alternate-faction model frames match, including inherited colour and texture swaps. A further 143 cases encounter the original unsafe lighting/texture reads already described above.
- Eight isolated window checks cover mouse navigation, Escape, F1, Quickstart dismissal and resized integer-scaled presentation.

These menu comparisons use a defined initial palette and initialized widgets. They do not yet cover the terrain preview, populated campaign/pilot state or every interactive menu action.

Next: scenery, moving objects, clouds and complete scene rendering; then simulation, avionics, frontend/campaign integration and audio. Full journey tests and CI follow once the game is connected.

Original uninitialized memory and out-of-bounds effects are excluded. Invalid resources and unsafe states receive bounded handling. No game media or generated screenshots are committed.

The reference engine loads and projects super-feature vertices but never submits their faces. The resource inspection commands can render those records; the game scene preserves their absence.
