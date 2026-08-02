import { describe, expect, it } from 'vitest'
import {
  SettingsSchema,
  settingsDefaults
} from '../../../elle-electron/settings-schema'

describe('settings schema', () => {
  it('produces complete defaults from an empty object', () => {
    const s = SettingsSchema.parse({})
    expect(s.diameterMode).toBe(false)
    expect(s.defaultMetricOnStartup).toBe(true)
    expect(s.encoderScaleZ).toBe(0.001)
    expect(s.encoderScaleX).toBe(-0.001)
    expect(s.selectedPitchTab).toEqual([0, 0])
    expect(s.tools).toHaveLength(10)
    expect(s.tools[0].description).toBe('Reference Tool')
  })

  it('recovers per-field from corrupt stored values', () => {
    const s = SettingsSchema.parse({
      diameterMode: 'yes',        // wrong type
      pitchX: 1.5,                // valid — must survive
      encoderScaleZ: 'broken',    // wrong type
      selectedPitchTab: [1, 2, 3] // wrong length
    })
    expect(s.diameterMode).toBe(false)
    expect(s.pitchX).toBe(1.5)
    expect(s.encoderScaleZ).toBe(0.001)
    expect(s.selectedPitchTab).toEqual([0, 0])
  })

  it('strips unknown keys (appBounds never leaks into the frontend)', () => {
    const s = SettingsSchema.parse({ appBounds: { x: 0, y: 0 }, pitchZ: 2 })
    expect('appBounds' in s).toBe(false)
    expect(s.pitchZ).toBe(2)
  })

  it('round-trips a full settings object unchanged', () => {
    const original = settingsDefaults()
    original.pitchX = 1.25
    original.tools[3].offsetX = -0.5
    expect(SettingsSchema.parse(original)).toEqual(original)
  })

  it('returns a fresh object from settingsDefaults (no shared state)', () => {
    const a = settingsDefaults()
    a.tools[0].offsetX = 99
    expect(settingsDefaults().tools[0].offsetX).toBe(0)
  })
})
