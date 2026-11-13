# Golden cycle data

`cycles.json` pins the behaviour of the original implementation, which the C++ core must reproduce
exactly. It was generated from commit `a587f56` by running the original code itself, not a re-port:

1. `oracle/derive.ts` bundles the original `elle-app/elle-frontend/src/composables/useCannedCycles.ts`
   and runs its planners (`generateThreadingParams`, `generateTurningParams`) for every shipped
   thread and turning preset, at two tool positions each. The values recorded under `request` are
   what the frontend's parameter refs held after its own rounding.
2. `oracle/expand.py` extracts `generate_threading_gcode_core` and `generate_turning_gcode_core`
   from the original `elle-app/elle-hal/lathe_halcomp.py` (without importing HAL or LinuxCNC) and
   produces the machine (`for_backplot=False`) and preview (`for_backplot=True`) programs.
3. `oracle/reshape.py` renames keys to the C++ field names.

```sh
esbuild oracle/derive.ts --bundle --platform=node --format=esm --outfile=derive.mjs \
  --alias:primevue/usedialog=./oracle/stub-usedialog.ts --external:*.vue
node derive.mjs threadpresets.json turningpresets.json > derived.json
python3 -I oracle/expand.py lathe_halcomp.py derived.json golden.json
python3 -I oracle/reshape.py golden.json cycles.json
```

The scripts import the original code, which was removed after the rewrite; run them from a checkout
of `a587f56` (for example `git worktree add /tmp/elle-original a587f56`).

## Preset data fixed after the golden data was recorded

The cases in `cycles.json` record the planner's *inputs* as well as its outputs, so they keep
pinning the planner exactly. Some of those inputs came from shipped preset data that was wrong;
the presets have since been fixed (`ui/src/presets/turningpresets.json`, checked by
`ui/test/presets.test.ts`), so picking these presets no longer produces the recorded requests:

- **Imperial turning presets:** `feedRate`, `stepDown` and `finalStepDown` were inches while every
  other value is mm, so they were applied 25.4x too small (e.g. 3/8": feed 0.006, step 0.02).
  They are now mm (0.1524 mm/rev, 0.508 mm).
- **Imperial turning targets:** 3/4", 1" and 1-1/4" turned to the radius of the next size up
  (12.7, 15.88, 19.05 mm); now 9.525, 12.7 and 15.875 mm.
- **NPT turning presets:** the 2A targets were undersized (⅛-27: Ø8.61, below the thread's major
  diameter, leaving almost no thread) and shorter than the matching thread (½-14: turned 14.2 mm,
  threaded 19.05 mm, so the thread ran into unturned stock). Targets are now the small-end major
  diameter E0 + 0.8p (ASME B1.20.1), lengths match the threading presets, taper 1.7899° as the
  threads. The 2B (internal) presets were removed: the turning cycle only turns outside diameters
  (validation rejects a target above the tool's radius), so they could never run.

## Thread pull-out direction (operator form)

The recorded threading requests carry the original's positive `z_pullout` (presets: 1.0), so the
pull-out `G33` runs back toward +Z after cutting toward the chuck (e.g. `Z-6` then `Z-5`): a
spindle-synchronized reversal that drags an opposite-hand helix across the last millimetre of the
thread. The planner still reproduces that for those requests, but the panel no longer sends them:
it sends a `ThreadingForm`, and `core::threading_request` (`src/core/src/cycle_form.cpp`) makes the
run-out continue toward the chuck and finish exactly at the thread end (tested in
`test/cycle_form_test.cpp`: no reversal, the deepest point at -Length).
