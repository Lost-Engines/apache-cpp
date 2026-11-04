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
- Localized mission briefings, ordered mission tags, tasks, actor definitions, routes, objective flags, environment settings and embedded formations.
- Actor classes and loadouts, formation expansion, variable model attachments, carrier reservations and complete starting poses.
- All weapon definitions, projectile acceleration/drag/expiry, blast and armour damage, AI weapon selection, firing permissions and difficulty-dependent acquisition/burst timing.
- Ground-vehicle and train movement, formation transitions, waypoint overshoot, battle stops, retreat and resumed advance.
- AI aircraft/ship steering, bank and pitch limits, terrain/obstacle avoidance, attack-speed selection and helicopter approach/landing bounds.
- Mission actor movement, patrol routes, carrier takeoff, retreat orders and model-tree synchronization.
- Scenery, mobile-object, recursive-box and crater collision queries.
- Laser, radar and infrared seekers, designation, decoys and lock cooldowns.
- Projectile guidance, lead estimates, lateral acceleration limits, gravity, proximity detonation and subdivided terrain impacts.

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
- 2,000 angular-rate and aerodynamic cases agree with the original; all integer and event fields are exact, with maximum double differences of 1.14e-13.
- 32 continuous realistic-flight traces span all four warzones, including carrier starts, takeoff, turns, gusts, engine failures, fuel exhaustion, hover, terrain following, variable frame timing and hard landings. All 2,997,072 integer/event fields agree across 76,832 states. Of 8,451,520 double fields, 8,264,472 match bit-for-bit; the maximum difference is 1.14e-13. Positions, attitude matrices, instruments and warning order remain exact over the complete traces.
- These flight comparisons also pass under AddressSanitizer and UndefinedBehaviorSanitizer. AI, collision-volume checks and the surrounding mission loop are separate work.
- 6,000 Arcade component cases match 337,859 fields exactly, covering input shaping, attitude limits, motion, fuel, heat/failures and hover.
- 28 continuous Arcade traces across all four warzones match 2,285,830 integer/event fields across 67,228 states. Of 4,235,364 double fields, 4,207,780 match bit-for-bit; the largest difference is 3.64e-12. The traces include carrier starts, takeoff, terrain-relative altitude holding, engine failures, fuel exhaustion, hover, variable timing and landings. The component and continuous sweeps also pass with both sanitizers enabled.
- All 411 briefing entries in four language bundles, 89 formations (803 members), and all 110 missions match the original-loader exports. This covers 1,871 tasks, 8,944 actor definitions, 21,989 expanded model IDs, 42,054 waypoints and 156,552 setting records. The complete loading comparison also passes under both sanitizers. Actor placement and running behavior are verified separately.
- All 1,315 actor type records match 19,725 fields. Across every mission, 58,496 model instances and attachment trees match, together with all 263,868 starting transform fields for 21,989 units. A second direct execution comparison matches 483,758 actor-state fields, including ammunition, angles, velocities, route selection, carrier slots and random ordering. Both full suites also pass under sanitizers.
- All 55 weapon definitions match original initialization. Another 67,142 fields match across 3,706 atmosphere queries, 12 difficulty rows, 8,640 speed updates, 2,160 expiry cases, 6,912 damage cases, 4,800 weapon selections, 4,000 firing decisions, 4,320 fire intervals and 600 acquisition delays. These include multiplayer damage rules, range boundaries, motor burnout and the original random sequence. The complete comparison also passes under sanitizers. Guidance, collision and live target acquisition remain separate integration work.
- Eight continuous ground-movement traces cover all four warzones, variable frame durations, destroyed formation members and retreat/advance orders. Across 19,200 updates, 212,080 states match 4,843,050 defined fields, including 5,594 waypoint crossings. Both normal and sanitizer builds pass. The original leaves a stopped formation member's route distance uninitialized; C++ sets it to zero.
- 6,000 AI steering cases match 144,000 output fields across aircraft, helicopters and ships, including terrain/obstacle avoidance, world-edge departures, attack speeds, acceleration easing and landing limits. The full sweep passes under sanitizers. The original turn-limitation flag can read unset locals on aligned/no-pitch paths; C++ uses zero corrections and those undefined flag bits are excluded from comparison.
- Eight complete mission-movement traces across all four warzones match 348,486 actor states and 9,025,846 defined fields over 19,200 updates. These include 5,594 waypoint crossings, 904 aircraft patrol loops, twelve carrier takeoffs, formation changes and retreat/advance orders. Normal and sanitizer builds agree. Player flight, live target acquisition/firing and scenery obstruction queries remain separate boundaries in these traces.
- 180,000 geometry/intersection cases match original execution: 6,000 perpendicular-distance calculations, 6,000 intercept estimates, and 24,000 cases each for scenery points/lines, mobile points/lines, recursive bounding boxes and crater points/lines. The checks include all four warzones, extra mission scenery, moving objects, ignored objects, edge points and overflow boundaries. Normal and sanitizer builds agree.
- 12,000 laser/radar/infrared seeker cases and 24,192 decoy cases match 129,577 fields, including lock flags, adjusted targets, cooldown deadlines, random-state updates and ordered visibility/random queries. Both normal and sanitizer builds pass. Target properties and visibility results are controlled inputs; world targeting is still being connected.
- 12,000 projectile movement cases, 6,000 closest-approach calculations and 12,000 terrain-bound queries match 431,607 defined fields, including 131 proximity detonations and 5,365 terrain hits. Normal and sanitizer builds agree. Target queries are controlled and damage/notification calls are observed; complete projectile lifecycles are the next integration step.
- 6,000 uniform and 6,000 weighted terrain visibility cases match 139,307 height samples. Another 24,000 complete line-of-sight cases across all four warzones match terrain, mobile and scenery queries and their final visibility decisions. The original leaves the obstruction position uninitialized when its first sample is underground; the reconstruction returns the start position.
- 11,999 projectile construction cases match 418,385 fields across all four movement classes, including dispersion, launch speed, flame attachments, random ordering and ballistic impact deadlines. One original path uses an unset roll angle; the reconstruction leaves that rotation unchanged.
- Continuous projectile-pool comparisons cover 4,800 updates and 1,200 launches across all four warzones. They match 5,782 active-round snapshots, 370,922 smoke records, 5,513 incoming-missile notifications, 235 mobile hits, 137 scenery hits, 456 terrain hits, 23 proximity detonations, 224 scheduled shell impacts, 57 map-edge removals and 44 expirations. Target kinematics remain controlled inputs and damage calls are observed; terrain, models, collision queries, pool reuse and smoke run normally.
- Projectile-capacity checks cover 259 successful allocations, fifteen rejected allocations and reuse of slots 0, 31 and 255. A failed allocation leaves the active count unchanged, avoiding the original incorrect count increment.
- 16,000 live-target descriptions, 4,000 target-equivalence cases and 4,000 sector-range checks match the original. These cover scenery, actors, projectiles and fixed points, cached classification, player speed conversion, radar emissions, laser designation and countermeasure deadlines. User-name lookup is controlled; model metadata, trigonometry and terrain are real inputs.
- The visibility, construction, continuous projectile, capacity and live-target comparisons all pass under AddressSanitizer and UndefinedBehaviorSanitizer.
- 960 live missile-tracking sequences across all four warzones match 15,360 original updates, including 392 decoy-target states and 91 retained targets after selection disappears. The comparison combines original target properties, laser retargeting, radar/infrared seekers, real terrain visibility, countermeasure deadlines and random state. All target bindings and classification caches agree. Normal and sanitizer builds pass; movement and damage are separate comparisons.
- 16,000 countermeasure cases match 235,472 original fields, covering AI release delays, player ammunition and pylon counts, chaff/flare cooldowns, incoming-missile warnings, automatic release and retreat timing. Ordered callbacks include 1,756 launches and 1,578 warnings. Both sanitizer and normal builds pass. Projectile construction and network/UI notification handlers are controlled boundaries.
- 12,000 direct ground/mobile damage cases and 12,000 player subsystem-failure cases match 220,224 original fields, including 2,669 destruction decisions and 12,287 feedback events. Real model/weapon metadata and RNGs are used; kill/effect/log callbacks are observed separately. Normal and sanitizer builds pass.
- 20,000 ordered blast-area searches across all four warzones match 367,137 ground-object results and 101,119 mobile results, including mission scenery, strict distance boundaries and changing sector lists. Both normal and sanitizer builds pass.
- 640 complete direct-hit/area-blast sequences across all four warzones match state hashes, random seeds and ordered effects, including 1,215 scenery destructions and 619 mobile destructions. These combine live area searches, direct damage, accumulated blast limits and destroyed-model replacement. Kill callbacks apply state changes; scoring, network and visual-effect handlers remain observed boundaries. Normal and sanitizer builds pass.
- Combat-driven effects match 3,600 creation operations, 1,060 updates, 6,188 random placement checks and 2,400 positional-audio calculations across all four maps. The state comparisons include 926,452 active explosion/crater records, partial and complete object destruction, weapon impacts, land/water crater choice, ring replacement and exact random ordering. Normal and sanitizer builds pass; audio playback is the sole hook.
- Actors can register their owned model trees directly with the mobile scene, keeping collision, targeting and animation on the same state. Carrier synchronization now copies every animated child and its maintained world transform. All 110 mission initializations and 3,056 scene operations still match; ownership/attachment checks and both 2,400-step carrier traces pass under sanitizers.
- Eight isolated window checks cover mouse navigation, Escape, F1, Quickstart dismissal and resized integer-scaled presentation.

These menu comparisons use a defined initial palette and initialized widgets. They do not yet cover the terrain preview, populated campaign/pilot state or every interactive menu action.

Next: connect damage and countermeasure responses to the projectile simulation, connect collision volumes to player flight, then complete simulation, frontend/campaign interaction and audio. Full journey tests and CI follow once the game is connected.

Original uninitialized memory and out-of-bounds effects are excluded. Invalid resources and unsafe states receive bounded handling. No game media or generated screenshots are committed.

The reference engine loads and projects super-feature vertices but never submits their faces. The resource inspection commands can render those records; the game scene preserves their absence.

Embedded resources currently use the January 1998 software `APACHE95.EXE`, loaded from `SOFTWARE/APACHE95.EXE` on the selected CD or from the installation root. Other executable layouts are not silently interpreted using those offsets.
