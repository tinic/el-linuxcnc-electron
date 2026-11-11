// The shipped presets, typed. Lengths are in mm; turning targets are radii.
import pitchData from './presets/manualpresets.json'
import threadData from './presets/threadpresets.json'
import turningData from './presets/turningpresets.json'

export interface PresetSection<T> {
  header: string
  items: T[]
  /** Index of the first item of a second group (e.g. internal threads), drawn after a rule. */
  splitAt?: number
}

export interface PitchPreset {
  name: string
  value: number
  type: 'pitch' | 'angle'
}

export interface ThreadPreset {
  name: string
  description: string
  Pitch: number
  Diameter: string
  XDepth: number
  ZDepth: number
  Angle: number
  ZEnd: number
  XPullout: number
  ZPullout: number
  FirstCut: number
  CutMult: number
  MinCut: number
  SpringCuts: number
}

export interface TurningPreset {
  name: string
  description: string
  target: number
  zEnd: number
  feedRate: number
  finalStepDown: number
  taperAngle: number
  stepDown: number
  springPasses?: number
}

interface PitchFile {
  axis: string
  sections: { header: string; pitches: PitchPreset[] }[]
}

export function pitchSections(axis: 'x' | 'z'): PresetSection<PitchPreset>[] {
  const file = (pitchData as PitchFile[]).find((entry) => entry.axis === axis)
  return (file?.sections ?? []).map((section) => ({ header: section.header, items: section.pitches }))
}

/** Index of the first internal thread (6H / -2B) following external ones (6g / -2A). */
function internalSplit(items: { name: string }[], external: string[], internal: string[]): number | undefined {
  const index = items.findIndex(
    (item, i) =>
      i > 0 &&
      internal.some((mark) => item.name.includes(mark)) &&
      external.some((mark) => items[i - 1]?.name.includes(mark)),
  )
  return index > 0 ? index : undefined
}

function section<T extends { name: string }>(
  header: string,
  items: T[],
  external: string[],
  internal: string[],
): PresetSection<T> {
  const splitAt = internalSplit(items, external, internal)
  return splitAt === undefined ? { header, items } : { header, items, splitAt }
}

const threads = threadData as Record<'metric' | 'imperial' | 'special', ThreadPreset[]>
export const threadSections: PresetSection<ThreadPreset>[] = [
  section('Metric', threads.metric, ['6g', '-2A'], ['6H', '-2B']),
  section('Imperial', threads.imperial, ['6g', '-2A'], ['6H', '-2B']),
  section('NPT', threads.special, ['6g', '-2A'], ['6H', '-2B']),
]

const turnings = turningData as Record<'metric' | 'imperial' | 'npt', TurningPreset[]>
export const turningSections: PresetSection<TurningPreset>[] = [
  section('Metric', turnings.metric, ['-2A'], ['-2B']),
  section('Imperial', turnings.imperial, ['-2A'], ['-2B']),
  section('NPT', turnings.npt, ['-2A'], ['-2B']),
]
