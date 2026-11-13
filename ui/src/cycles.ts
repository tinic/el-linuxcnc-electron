// Canned-cycle entries as the operator makes them, kept across page switches. Values are mm (X as
// radius) or plain numbers; null means "use the default". The backend converts them into the
// planner's requests (signs, infeed, thread end), see backend/src/core/src/cycle_form.cpp.

import { reactive } from 'vue'

import type { ThreadPreset, TurningPreset } from './presets'
import type { ThreadingForm, TurningForm } from './protocol'

/** How a parameter is shown and entered. */
export type ParamKind = 'length' | 'diameter' | 'angle' | 'ratio' | 'count'

export interface ParamField<Key extends string> {
  key: Key
  label: string
  placeholder: string
  help: string
  kind: ParamKind
}

export type ThreadingKey = Exclude<keyof ThreadingForm, 'internal'>
export type TurningKey = keyof TurningForm

// Two columns, read left to right; null leaves a slot empty.
export const threadingFields: (ParamField<ThreadingKey> | null)[] = [
  { key: 'pitch', label: 'Pitch', placeholder: 'Pitch', kind: 'length', help: 'Distance from one thread to the next.' },
  { key: 'length', label: 'Length', placeholder: 'Length', kind: 'length', help: 'Where the thread ends, measured from the face (Z0) into the work. It starts at the tool.' },
  { key: 'depth', label: 'Depth', placeholder: 'auto', kind: 'length', help: 'Thread depth (radius). Empty: from the pitch, 0.613 × P external and 0.541 × P internal (60° threads).' },
  { key: 'taper_deg', label: 'Taper ∠', placeholder: '0', kind: 'angle', help: 'Taper angle in degrees. 0 for straight threads; NPT 1.7899.' },
  { key: 'infeed_deg', label: 'Infeed ∠', placeholder: '0', kind: 'angle', help: 'Infeed angle: 0 feeds straight in, 29.5 feeds along the flank of a 60° thread so only the leading edge cuts.' },
  { key: 'spring_cuts', label: 'Spring', placeholder: '1', kind: 'count', help: 'Extra passes at full depth (0 to 10).' },
  { key: 'first_cut', label: 'First cut', placeholder: '0.1', kind: 'length', help: 'Depth of the first pass.' },
  { key: 'min_cut', label: 'Min cut', placeholder: '0.05', kind: 'length', help: 'Passes never get shallower than this.' },
  { key: 'cut_factor', label: 'Factor', placeholder: '0.8', kind: 'ratio', help: 'Cut factor: each pass is the previous one times this (0.5 to 1.0).' },
  null,
  { key: 'run_out', label: 'Run-out', placeholder: '1 pitch', kind: 'length', help: 'The tool pulls out at an angle over this distance, finishing at the thread end. Empty: one pitch.' },
  { key: 'retract', label: 'Retract', placeholder: '0.5', kind: 'length', help: 'Clearance from the thread for the rapid return.' },
]

export const turningFields: (ParamField<TurningKey> | null)[] = [
  { key: 'target', label: 'Target', placeholder: 'Target', kind: 'diameter', help: 'Finished size at the face, as a diameter in diameter mode. Must be below the stock at the tool position.' },
  { key: 'length', label: 'Length', placeholder: 'Length', kind: 'length', help: 'Where the cut ends, measured from the face (Z0) into the work.' },
  { key: 'feed_rate', label: 'Feed', placeholder: '0.2', kind: 'length', help: 'Feed per spindle revolution.' },
  { key: 'taper_angle_deg', label: 'Taper ∠', placeholder: '0', kind: 'angle', help: 'Taper angle in degrees; the size grows toward the chuck. 0 for straight turning.' },
  { key: 'step_down', label: 'Cut depth', placeholder: '0.5', kind: 'length', help: 'Depth of each roughing pass (radius).' },
  { key: 'final_step_down', label: 'Finish cut', placeholder: '0.1', kind: 'length', help: 'Depth of the finishing pass (radius).' },
  { key: 'spring_passes', label: 'Spring', placeholder: '0', kind: 'count', help: 'Extra finishing passes at full depth (0 to 10).' },
]

function emptyThreading(): ThreadingForm {
  return {
    pitch: null,
    internal: false,
    depth: null,
    infeed_deg: null,
    length: null,
    taper_deg: null,
    run_out: null,
    retract: null,
    first_cut: null,
    cut_factor: null,
    min_cut: null,
    spring_cuts: null,
  }
}

function emptyTurning(): TurningForm {
  return {
    target: null,
    length: null,
    feed_rate: null,
    step_down: null,
    final_step_down: null,
    taper_angle_deg: null,
    spring_passes: null,
  }
}

interface CycleEntries {
  selected: 'threading' | 'turning'
  threading: ThreadingForm
  threadingPreset: { name: string; diameter: string } | null
  turning: TurningForm
  turningPreset: string | null
}

export const cycles = reactive<CycleEntries>({
  selected: 'threading',
  threading: emptyThreading(),
  threadingPreset: null,
  turning: emptyTurning(),
  turningPreset: null,
})

export function setThreading(key: ThreadingKey, value: number | null): void {
  cycles.threading[key] = value
  cycles.threadingPreset = null
}

/** External or internal thread. A preset is for one side, so choosing the other forgets it. */
export function setThreadSide(internal: boolean): void {
  if (cycles.threading.internal !== internal) {
    cycles.threading.internal = internal
    cycles.threadingPreset = null
  }
}

export function setTurning(key: TurningKey, value: number | null): void {
  cycles.turning[key] = value
  cycles.turningPreset = null
}

export function resetThreading(): void {
  Object.assign(cycles.threading, emptyThreading())
  cycles.threadingPreset = null
}

export function resetTurning(): void {
  Object.assign(cycles.turning, emptyTurning())
  cycles.turningPreset = null
}

/** Degrees, rounded to 0.01: the infeed angle a preset's compound depths imply. */
function infeedAngle(xDepth: number, zDepth: number): number {
  return xDepth === 0 ? 0 : Math.round((Math.atan(Math.abs(zDepth / xDepth)) * 18000) / Math.PI) / 100
}

/**
 * The presets keep the original's signed planner values, with the tool at the face (ZEnd measured
 * from Z0); they are entered here in the form's terms.
 */
export function applyThreadPreset(preset: ThreadPreset): void {
  Object.assign(cycles.threading, {
    pitch: preset.Pitch,
    internal: preset.XDepth > 0,
    depth: Math.abs(preset.XDepth),
    infeed_deg: infeedAngle(preset.XDepth, preset.ZDepth),
    length: Math.abs(preset.ZEnd),
    taper_deg: preset.Angle,
    run_out: Math.abs(preset.ZPullout),
    retract: Math.abs(preset.XPullout),
    first_cut: preset.FirstCut,
    cut_factor: preset.CutMult,
    min_cut: preset.MinCut,
    spring_cuts: preset.SpringCuts,
  } satisfies ThreadingForm)
  cycles.threadingPreset = { name: preset.name, diameter: preset.Diameter }
}

export function applyTurningPreset(preset: TurningPreset): void {
  Object.assign(cycles.turning, {
    target: preset.target,
    length: Math.abs(preset.zEnd),
    feed_rate: preset.feedRate,
    step_down: preset.stepDown,
    final_step_down: preset.finalStepDown,
    taper_angle_deg: preset.taperAngle,
    spring_passes: preset.springPasses ?? 0,
  } satisfies TurningForm)
  cycles.turningPreset = preset.name
}
