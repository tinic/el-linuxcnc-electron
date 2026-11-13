import { describe, expect, it } from 'vitest'

import { cuttingSide, envelope, niceStep, passNumbers, segmentAt } from '../src/backplot'
import type { Segment } from '../src/protocol'

function segment(kind: Segment['kind'], from: [number, number], to: [number, number]): Segment {
  return { kind, from: { x: from[0], z: from[1] }, to: { x: to[0], z: to[1] }, line: 0 }
}

// Two turning passes from X5 at Z2 down to X4.5, then X4, cutting to Z-10.
const turning: Segment[] = [
  segment('rapid', [5, 2], [4.5, 2]),
  segment('feed', [4.5, 2], [4.5, -10]),
  segment('rapid', [4.5, -10], [7, -10]),
  segment('rapid', [7, -10], [7, 2]),
  segment('rapid', [7, 2], [4, 2]),
  segment('feed', [4, 2], [4, -10]),
  segment('rapid', [4, -10], [7, -10]),
]

describe('backplot geometry', () => {
  it('tells external from internal cuts', () => {
    expect(cuttingSide(turning, { x: 5, z: 2 })).toBe('external')
    const boring = [segment('feed', [6, 2], [6, -10])]
    expect(cuttingSide(boring, { x: 5, z: 2 })).toBe('internal')
  })

  it('takes the deepest cut as the finished profile', () => {
    const profile = envelope(turning, 'external', -12, 0, 13)
    expect(profile.at(0)?.x).toBeNull() // beyond the end of the cut
    expect(profile.at(-1)?.x).toBe(4)
    expect(profile.find((point) => point.z === -5)?.x).toBe(4)
  })

  it('numbers passes by runs of cuts', () => {
    const { pass, count } = passNumbers(turning)
    expect(count).toBe(2)
    expect(pass).toEqual([0, 0, 1, 1, 1, 1, 2])
  })

  it('picks 1, 2 or 5 times a power of ten', () => {
    expect(niceStep(0.7)).toBe(1)
    expect(niceStep(1.3)).toBe(2)
    expect(niceStep(3)).toBe(5)
    expect(niceStep(7)).toBe(10)
    expect(niceStep(0.013)).toBe(0.02)
  })

  it('follows the tool forward along retraced passes', () => {
    // On the second pass the tool is on the second feed even though the first one is as close.
    expect(segmentAt(turning, { x: 4, z: -3 }, 4)).toBe(5)
    expect(segmentAt(turning, { x: 4.5, z: -3 }, 0)).toBe(1)
  })
})
