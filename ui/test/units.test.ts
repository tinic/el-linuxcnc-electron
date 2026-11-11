import { describe, expect, it } from 'vitest'

import { formatLength, pitchLabel, xFromDisplay, xToDisplay } from '../src/units'

describe('units', () => {
  it('shows X as a diameter in diameter mode and back', () => {
    expect(xToDisplay(5, true, true)).toBe(10)
    expect(xFromDisplay(10, true, true)).toBe(5)
    expect(xToDisplay(5, true, false)).toBe(5)
  })

  it('converts inches both ways', () => {
    expect(xToDisplay(12.7, false, true)).toBeCloseTo(1)
    expect(xFromDisplay(1, false, true)).toBeCloseTo(12.7)
  })

  it('formats mm with three and inches with four decimals', () => {
    expect(formatLength(1.23456, true)).toBe('1.235')
    expect(formatLength(25.4, false)).toBe('1.0000')
  })

  it('labels pitches like the presets do', () => {
    expect(pitchLabel(1.5, true)).toBe('1.5mm')
    expect(pitchLabel(25.4 / 18, false)).toBe('18 TPI')
    expect(pitchLabel(25.4 / 11.5, false)).toBe('11.5 TPI')
  })
})
