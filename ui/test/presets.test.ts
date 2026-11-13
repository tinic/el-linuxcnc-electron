// Sanity checks on the shipped preset data: every value is in mm (mm/rev for feeds), and sizes match
// the preset's own name. The original data had imperial turning feeds and steps in inches (read as
// mm, 25x too small) and three imperial targets shifted by one size.
import { describe, expect, it } from 'vitest'

import threadData from '../src/presets/threadpresets.json'
import turningData from '../src/presets/turningpresets.json'

const inch = 25.4

/** Parses "3/8", "1", "1-1/4" (inches). */
function inches(text: string): number {
  const [whole, fraction] = text.includes('-') ? text.split('-') : text.includes('/') ? ['0', text] : [text, '0/1']
  const [numerator, denominator] = (fraction ?? '0/1').split('/').map(Number)
  return Number(whole) + (numerator ?? 0) / (denominator ?? 1)
}

describe('turning presets', () => {
  it('turn metric presets to the diameter in their name', () => {
    for (const preset of turningData.metric) {
      const diameter = Number(/^(\d+(?:\.\d+)?)mm$/.exec(preset.name)?.[1])
      expect(preset.target * 2, preset.name).toBeCloseTo(diameter, 6)
    }
  })

  it('turn imperial presets to the diameter in their name', () => {
    for (const preset of turningData.imperial) {
      const size = /^([\d/-]+)"$/.exec(preset.name)?.[1] ?? ''
      expect(preset.target * 2, preset.name).toBeCloseTo(inches(size) * inch, 3)
    }
  })

  it('give feeds and steps in mm in every group', () => {
    for (const [group, presets] of Object.entries(turningData)) {
      for (const preset of presets) {
        const label = `${group}/${preset.name}`
        expect(preset.feedRate, label).toBeGreaterThanOrEqual(0.05)
        expect(preset.feedRate, label).toBeLessThanOrEqual(0.5)
        expect(preset.stepDown, label).toBeGreaterThanOrEqual(0.1)
        expect(preset.stepDown, label).toBeLessThanOrEqual(2)
        expect(preset.finalStepDown, label).toBeGreaterThanOrEqual(0.05)
        expect(preset.finalStepDown, label).toBeLessThanOrEqual(preset.stepDown)
        expect(preset.zEnd, label).toBeLessThan(0)
      }
    }
  })
})

describe('NPT turning presets', () => {
  const threads = new Map(threadData.special.map((preset) => [preset.name, preset]))

  it('turn the blank for the matching NPT thread: small-end major diameter, same taper and length', () => {
    for (const preset of turningData.npt) {
      const thread = threads.get(preset.name)
      expect(thread, preset.name).toBeDefined()
      if (thread === undefined) {
        continue
      }
      const outside = Number(thread.Diameter.split('/')[0])
      // Small-end major diameter: under the pipe OD, but no more than ~1.5 thread heights below it.
      expect(preset.target * 2, preset.name).toBeLessThan(outside)
      expect(preset.target * 2, preset.name).toBeGreaterThan(outside - 2 * Math.abs(thread.XDepth))
      expect(preset.taperAngle, preset.name).toBe(thread.Angle)
      expect(Math.abs(preset.zEnd), preset.name).toBeGreaterThanOrEqual(Math.abs(thread.ZEnd))
    }
  })

  it('only external (2A): the turning cycle turns outside diameters and cannot bore', () => {
    expect(turningData.npt.every((preset) => preset.name.endsWith('-2A'))).toBe(true)
  })
})

describe('thread presets', () => {
  it('use the pitch in the name of metric threads', () => {
    for (const preset of threadData.metric) {
      const pitch = Number(/^M[\d.]+x([\d.]+)-/.exec(preset.name)?.[1])
      expect(preset.Pitch, preset.name).toBeCloseTo(pitch, 6)
    }
  })

  it('convert the threads per inch in the name of imperial threads to a pitch in mm', () => {
    for (const preset of [...threadData.imperial, ...threadData.special]) {
      const tpi = Number(/-(\d+(?:\.\d+)?)[- ]/.exec(preset.name)?.[1])
      expect(preset.Pitch, preset.name).toBeCloseTo(inch / tpi, 2)
    }
  })

  it('give cut sizes in mm in every group', () => {
    for (const [group, presets] of Object.entries(threadData)) {
      for (const preset of presets) {
        const label = `${group}/${preset.name}`
        expect(preset.FirstCut, label).toBeGreaterThanOrEqual(0.05)
        expect(preset.FirstCut, label).toBeLessThanOrEqual(0.6)
        expect(preset.MinCut, label).toBeGreaterThan(0)
        expect(preset.MinCut, label).toBeLessThanOrEqual(preset.FirstCut)
        expect(Math.abs(preset.XDepth), label).toBeLessThan(preset.Pitch)
      }
    }
  })
})
