// Canned-cycle parameters as entered, kept across page switches. Values are mm (X as radius) or
// plain numbers; null means "use the default".

import { reactive } from 'vue'

import type { ThreadingRequest, TurningRequest } from './protocol'
import type { ThreadPreset, TurningPreset } from './presets'

/** How a parameter is shown and entered. */
export type ParamKind = 'length' | 'diameter' | 'angle' | 'ratio' | 'count'

export interface ParamField<Key extends string> {
  key: Key
  label: string
  placeholder: string
  help: string
  kind: ParamKind
}

export const threadingFields: (ParamField<keyof ThreadingRequest> | null)[] = [
  { key: 'pitch', label: 'Pitch', placeholder: 'Pitch', kind: 'length', help: 'Thread pitch: distance between threads.' },
  null, // the major diameter / tap drill column, display only
  { key: 'z_depth', label: 'ZDepth', placeholder: '0', kind: 'length', help: 'Longitudinal infeed depth for compound infeed. Zero for straight infeed.' },
  { key: 'x_depth', label: 'XDepth', placeholder: 'X Depth', kind: 'length', help: 'Cross-slide thread depth (radius). Negative for external, positive for internal threads.' },
  { key: 'z_end', label: 'ZEnd', placeholder: 'Z End', kind: 'length', help: 'Thread length from the tool position. Negative for right-hand threads.' },
  { key: 'angle_deg', label: 'Angle', placeholder: '0', kind: 'angle', help: 'Taper angle in degrees. Zero for straight threads.' },
  { key: 'z_pullout', label: 'ZPull', placeholder: '0.1', kind: 'length', help: 'Distance the tool keeps moving in Z while pulling out.' },
  { key: 'x_pullout', label: 'XPull', placeholder: '0.1', kind: 'length', help: 'Cross-slide retract distance. Positive for external, negative for internal threads.' },
  { key: 'cut_mult', label: 'Mult', placeholder: '0.8', kind: 'ratio', help: 'Each cut is the previous cut times this (0.5 to 1.0).' },
  { key: 'first_cut', label: '1stCut', placeholder: '0.1', kind: 'length', help: 'Depth of the first cut.' },
  { key: 'spring_cuts', label: 'Spring', placeholder: '1', kind: 'count', help: 'Finishing passes at full depth (0 to 10).' },
  { key: 'min_cut', label: 'MinCut', placeholder: '0.05', kind: 'length', help: 'Smallest cut depth; cuts never get shallower than this.' },
]

export const turningFields: (ParamField<keyof TurningRequest> | null)[] = [
  { key: 'target', label: 'Target', placeholder: 'Target', kind: 'diameter', help: 'Finished size, as a diameter in diameter mode. Must be below the stock at the tool position.' },
  null,
  { key: 'z_end', label: 'Length', placeholder: 'Length', kind: 'length', help: 'Z position the passes cut to, from the work face (Z0). Negative into the work.' },
  null,
  { key: 'feed_rate', label: 'Feed', placeholder: '0.2', kind: 'length', help: 'Feed per spindle revolution.' },
  { key: 'step_down', label: 'Step', placeholder: '0.5', kind: 'length', help: 'Depth of each roughing pass (radius).' },
  { key: 'spring_passes', label: 'Spring', placeholder: '0', kind: 'count', help: 'Extra finishing passes at full depth (0 to 10).' },
  { key: 'final_step_down', label: 'Final', placeholder: '0.1', kind: 'length', help: 'Depth of the finishing pass (radius).' },
  { key: 'taper_angle_deg', label: 'Taper', placeholder: '0', kind: 'angle', help: 'Taper angle in degrees. Zero for straight turning.' },
]

function emptyThreading(): ThreadingRequest {
  return {
    pitch: null,
    x_depth: null,
    z_depth: null,
    angle_deg: null,
    z_end: null,
    x_pullout: null,
    z_pullout: null,
    first_cut: null,
    cut_mult: null,
    min_cut: null,
    spring_cuts: null,
  }
}

function emptyTurning(): TurningRequest {
  return {
    target: null,
    z_end: null,
    feed_rate: null,
    step_down: null,
    final_step_down: null,
    taper_angle_deg: null,
    spring_passes: null,
  }
}

interface CycleEntries {
  selected: 'threading' | 'turning'
  threading: ThreadingRequest
  threadingPreset: { name: string; diameter: string } | null
  turning: TurningRequest
  turningPreset: string | null
}

export const cycles = reactive<CycleEntries>({
  selected: 'threading',
  threading: emptyThreading(),
  threadingPreset: null,
  turning: emptyTurning(),
  turningPreset: null,
})

export function setThreading(key: keyof ThreadingRequest, value: number | null): void {
  cycles.threading[key] = value
  cycles.threadingPreset = null
}

export function setTurning(key: keyof TurningRequest, value: number | null): void {
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

export function applyThreadPreset(preset: ThreadPreset): void {
  Object.assign(cycles.threading, {
    pitch: preset.Pitch,
    x_depth: preset.XDepth,
    z_depth: preset.ZDepth,
    angle_deg: preset.Angle,
    z_end: preset.ZEnd,
    x_pullout: preset.XPullout,
    z_pullout: preset.ZPullout,
    first_cut: preset.FirstCut,
    cut_mult: preset.CutMult,
    min_cut: preset.MinCut,
    spring_cuts: preset.SpringCuts,
  } satisfies ThreadingRequest)
  cycles.threadingPreset = { name: preset.name, diameter: preset.Diameter }
}

export function applyTurningPreset(preset: TurningPreset): void {
  Object.assign(cycles.turning, {
    target: preset.target,
    z_end: preset.zEnd,
    feed_rate: preset.feedRate,
    step_down: preset.stepDown,
    final_step_down: preset.finalStepDown,
    taper_angle_deg: preset.taperAngle,
    spring_passes: preset.springPasses ?? 0,
  } satisfies TurningRequest)
  cycles.turningPreset = preset.name
}
