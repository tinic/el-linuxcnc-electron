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

These scripts depend on the original code and exist only to document and reproduce how the data was
made. They are removed together with `elle-app/`.
