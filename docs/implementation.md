# Implementation and validation

The reconstruction follows the January 1998 Windows software engine using data shared by the September 1998 and October 2000 European discs. It reads the original resource formats directly. There is no CPU emulator, translated executable or dependency on the analysis workspace in the game.

The sibling analysis project supplies original-code reference runs and independent numerical tests. C++ comparisons are added as each subsystem is implemented. Passing original-against-original repeatability tests is not counted as C++ acceptance.

Current implementation:

- Bounded binary readers and case-insensitive resource lookup.
- Packed and planar IFF images with ByteRun1 compression and indexed palettes.
- Terrain heights and scenery placements.
- Model geometry, attachments, bounded drawing programs and authored animation branches.
- Original rounded sine/cosine, arcsine and arctangent lookup, with Q31 heading/pitch/roll matrices and angle recovery.
- Software line, circle, polygon, mesh, translucent, heat-haze, affine-textured and Gouraud-shaded rasterization.
- Complete model drawing with authored attachments, fixed-point grid coordinates, lighting, animation branches, level-of-detail chains and faction colour/texture swaps.
- Original palette, intensity ramps and glass remaps for all four warzones.
- Ordered palette fades, night/weather/glare transitions, grassy terrain colours and infrared/HUD remaps.
- Complete RGB scene output, including thermal-view detail and visibility rules.
- Bounded reads of embedded palette and instrument-font data from the supported original executable.
- All pilot/gunner cockpit panels, full and incremental redraws, night lighting and night-vision brightness.
- Both original instrument fonts, including every glyph and screen-edge clipping.
- Mechanical compass, artificial horizon, airspeed indicator, altimeter and analogue clock at both original resolutions.
- Pilot/gunner warning lamps, clipped radio text and time-acceleration indicator.
- Complete helmet-display modes, flight scales, sight-field diagrams, gun/rocket/missile aiming cues and target information.
- World-to-helmet target projection and flight/waypoint conversion to instrument values.
- MFD off/damaged, engine, system-status, weapons, radar, tactical, isolated-target FLIR, flight-plan and moving-map pages, with the original display bezels.
- Target-camera positioning, inherited thermal colour maps, selected-contact highlighting, automatic route scaling and waypoint arrival estimates.
- Independent pilot/gunner MFD selections, direction-aware cycling, power switches, radar requests and damage transitions.
- Complete single-player avionics composition in the original drawing order, with runtime model names and properties.
- Fuel consumption, engine/rotor response, autorotation terms, engine temperature and failure warnings.
- Flight-input normalization, automatic-hover corrections and touchdown grading.
- Fixed-point 2D line clipping, inverse projection and unclipped text positioning.
- Rotated terrain maps with contours, super-features, faction markers and model footprints.
- Terrain grid projection, 7,200-entry scene queue, texture selection and fog shading tables.
- Seven terrain drawing paths, including 64-triangle subdivision and perspective correction in 16-pixel spans.
- Complete terrain, feature and placed-scenery assembly, visible-sector selection, and the original ground/sky horizon.
- Star-map decoding, night visibility thresholds, sun/moon projection and sun glare.
- Cloud-sector selection, quantized cloud mesh, detail transitions, horizon fading and interleaved scene sorting.
- Smoke and explosion creation, circular storage, lifetime updates, masked billboards and scene sorting.
- Short/long crater pools, authored sizing, splash state changes and scaled crater model drawing.
- Detached model fragments with recursive attachment breakup, arbitrary-axis spin, gravity, ground resting and water splashes.
- Normal and super-feature material queries for land/water interactions.
- Moving-object sector lists, cloud visibility, model/shadow sorting and hidden-player handling.
- Sun-cast model shadows with clipping, attachments, detail chains and ship-deck receiving.
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
- 192 assembled landscape images match all pixels across the four warzones and terrain-quality levels. These include normal features, terrain sorting and the horizon. Another 80 landscape and 424 horizon comparisons pass under sanitizers.
- 2,485 alternate-faction model frames match, including inherited colour and texture swaps. A further 143 cases encounter the original unsafe lighting/texture reads already described above.
- 68 complete scenery frames match exactly in a 192-case run across all warzones. The remaining frames include the original unsafe model reads described above; all 192 C++ frames use bounded handling.
- 8,808 circle frames match 408,858,624 pixels, including edge clipping, mesh patterns and translucent remapping.
- 1,324 celestial frames match 171,545,600 pixels and every sun-glare value, including twilight thresholds, weather cover and camera roll.
- The full 192-case scenery run and another 424 celestial frames pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- 168 cloud-grid cases match 1,241,584 vertices and all 1,296 mesh-transition cells.
- 296 complete cloud images match 22,732,800 pixels and all 193,639 queued cloud objects.
- 192 assembled terrain/cloud/sky images match 14,745,600 pixels across all four warzones and weather states, including views inside and above clouds.
- Another 120 cloud frames and 160 assembled weather scenes pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- 1,824 smoke/explosion billboard frames match 140,083,200 pixels, including all 56 active textures, rotation and clipping.
- 4,635 effect operations match 411 original state snapshots containing 185,236 active records, including storage wraparound and expiration.
- 96 assembled effect scenes match 7,372,800 pixels across all four warzones and weather states.
- Effect sprite, state and combined-scene comparisons also run under AddressSanitizer and UndefinedBehaviorSanitizer.
- 3,392 shadow frames match 260,505,600 pixels across all 848 true-coordinate models, four orientations and animation states, and both ground and cloud-layer projection.
- 3,056 mobile-scene operations match 144 sector-list snapshots, 384 drawing queues and 400 ship-deck receiver checks.
- 96 assembled moving-object scenes match 7,372,800 pixels, including aircraft, carrier models, shadows, hidden-player handling, terrain, clouds, smoke and explosions.
- Shadow catalogue, mobile-state and combined-scene comparisons also pass under sanitizers.
- 4,168 crater operations match 360 snapshots containing 40,853 active records.
- 932 crater/splash frames match 71,577,600 pixels. The other 68 cases reach the original invalid normal references described above.
- 81 assembled crater scenes match 6,220,800 pixels; 15 further cases encounter those same original invalid reads. All C++ cases use bounded handling, with additional state/frame/scene sanitizer runs.
- 10,162 vector-normalization/arbitrary-axis cases match all 121,944 original fields.
- 44,384 terrain-material queries match across all four warzones, including distant super-features.
- 3,268 fragment operations match 792 snapshots with 36,638 active fragments and 3,180 splash records. These include circular replacement, attached-part separation and ground settling.
- 83 combined debris scenes match 6,374,400 pixels. The other 13 cases encounter the original invalid model reads already described above. Vector, material and fragment-state comparisons also pass under sanitizers, as do another 48 complete debris scenes.
- 2,400 palette compositions with 9,591 faders match all 1,843,200 RGB bytes.
- 2,304 scene-palette cases match 5,898,240 bytes across weather, altitude, night, glare and infrared settings.
- 84 complete RGB scenes match 6,451,200 pixels. Twelve further scenes contain the same original invalid model reads already identified above. The complete fader/palette checks and another 48 RGB scenes also pass under sanitizers.
- 1,280 cockpit-panel cases match 245,760,000 pixels and 983,040 palette bytes across both original resolutions, all views, redraw modes and lighting states.
- 1,560 instrument-font cases match 1,597,440 pixels, covering all 130 glyphs in both fonts and drawing at the edges of the screen.
- 2,532 mechanical-instrument frames match 486,144,000 pixels, covering both resolutions, dial limits, heading wraparound, clock rollover and normal/inverted attitudes.
- 10,980 2D line/text cases match 78,668 fields; the existing 6,245 polygon-clipping comparisons still match after sharing the transformation code.
- The complete cockpit, font, mechanical-instrument and 2D line comparisons also pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- 552 warning/radio/time-indicator frames match 105,984,000 pixels.
- 1,234 off/damaged/engine/status MFD frames match 236,928,000 pixels and every damaged-display random seed; four final bezel passes match another 768,000 pixels.
- 4,068 MFD-control cases match 20,340 state fields, including power-on page selection and fault handling. All these display and state checks also pass under sanitizers.
- 10,162 direction-matrix cases match all 121,944 coefficients and normalized-vector fields.
- 5,017 isolated-target FLIR frames match 964,300,800 pixels across both resolutions and both inherited colour maps. Another 249 frames reach the previously identified missing model normals. The complete FLIR and direction-matrix sweeps also pass under sanitizers.
- 2,240 weapons-panel frames match 430,080,000 pixels across loadouts, ammunition counts, arming and automatic countermeasure states.
- 2,000 radar/tactical frames match 384,000,000 pixels, covering contact symbols, highlighting, clipping, heading, failures, freeze, TADS and range labels.
- 4,452 circle-outline cases match 394,979,328 pixels, including all screen-edge clipping cases.
- 1,150 flight-plan frames match 220,800,000 pixels across route-scale boundaries, headings, active waypoints and airspeeds; 10,487 integer square roots match original 64-bit calculations.
- The complete weapons, radar/tactical, flight-plan, circle-outline and integer-square-root comparisons also pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- 744 complete maps match 141,465,600 pixels, including rotated views, whole warzones, contours, super-features and faction footprints. Another 640 moving-map MFD frames match 125,644,800 pixels.
- 3,080 isolated contour frames match 236,544,000 pixels; 5,000 inverse-projection cases match 60,000 coordinate fields.
- All 42,317 initialized scenery records match 296,219 fields, including 1,205 status changes made by the original initializer.
- The complete map, contour, inverse-projection and scenery-initialization checks also pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- 2,640 helmet flight-instrument frames match 506,880,000 pixels across all 20 views and both resolutions.
- 2,200 weapon-cue and complete helmet-layout frames match 422,400,000 pixels, including lock states, off-screen targets, damage and all six player weapons.
- 5,420 world-to-helmet projection cases match 8,326 output fields; 4,112 flight-to-instrument conversions match 57,568 fields.
- The complete helmet component, weapon/layout, target-projection and instrument-state comparisons also pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- 174 complete avionics passes use states and scene/panel inputs captured from original missions. All 147 frames without invalid original model reads match 25,113,600 pixels, together with display selections and random seeds. The remaining 27 frames use bounded missing-normal handling. This compares the full avionics pass; the original simulation and scene remain reference inputs.
- All 1,314 runtime model property records match 13,140 fields after the original weapon-data initializer.
- 9,602 flight-subsystem cases match 446,620 fields exactly, including binary64 values, fault flags and ordered warning/audio events. These cover fuel, engines, temperature, failures, controls, hover and landing.
- The complete avionics, runtime-property and flight-subsystem comparisons also pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- Eight isolated window checks cover mouse navigation, Escape, F1, Quickstart dismissal and resized integer-scaled presentation.

These menu comparisons use a defined initial palette and initialized widgets. They do not yet cover the terrain preview, populated campaign/pilot state or every interactive menu action.

Next: aerodynamic integration, live sensor/target behavior, complete simulation, frontend/campaign interaction and audio. Full journey tests and CI follow once the game is connected.

Original uninitialized memory and out-of-bounds effects are excluded. Invalid resources and unsafe states receive bounded handling. No game media or generated screenshots are committed.

The reference engine loads and projects super-feature vertices but never submits their faces. The resource inspection commands can render those records; the game scene preserves their absence.

Embedded resources currently use the January 1998 software `APACHE95.EXE`, loaded from `SOFTWARE/APACHE95.EXE` on the selected CD or from the installation root. Other executable layouts are not silently interpreted using those offsets.
