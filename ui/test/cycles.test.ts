import { beforeEach, describe, expect, it } from 'vitest'

import { applyThreadPreset, applyTurningPreset, cycles, resetThreading, setThreadSide } from '../src/cycles'
import { threadSections, turningSections } from '../src/presets'

const threads = threadSections.flatMap((section) => section.items)
const turnings = turningSections.flatMap((section) => section.items)

describe('presets in the form', () => {
  beforeEach(resetThreading)

  it('enter every thread preset as positive numbers with the side chosen', () => {
    for (const preset of threads) {
      applyThreadPreset(preset)
      const form = cycles.threading
      expect(form.internal, preset.name).toBe(preset.XDepth > 0)
      for (const key of ['depth', 'length', 'run_out', 'retract'] as const) {
        expect(form[key], `${preset.name} ${key}`).toBeGreaterThan(0)
      }
      expect(form.infeed_deg, preset.name).toBe(0)
    }
  })

  it('mark 6H/2B presets internal and 6g/2A presets external', () => {
    const internal = threads.find((preset) => preset.name === 'M6x1-6H')
    const external = threads.find((preset) => preset.name === 'M6x1-6g')
    expect(internal).toBeDefined()
    expect(external).toBeDefined()
    if (internal && external) {
      applyThreadPreset(internal)
      expect(cycles.threading.internal).toBe(true)
      applyThreadPreset(external)
      expect(cycles.threading.internal).toBe(false)
    }
  })

  it('forget the preset when the other side is chosen', () => {
    const external = threads.find((preset) => preset.name === 'M6x1-6g')
    if (external) {
      applyThreadPreset(external)
      setThreadSide(true)
      expect(cycles.threadingPreset).toBeNull()
    }
  })

  it('enter turning lengths as positive distances from the face', () => {
    for (const preset of turnings) {
      applyTurningPreset(preset)
      expect(cycles.turning.length, preset.name).toBeCloseTo(-preset.zEnd, 9)
    }
  })
})
