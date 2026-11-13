// Geometry for drawing a backplot: which side the cycle cuts, the finished profile it leaves, how
// its segments group into passes, tick steps, and where a live tool is along the path. Pure
// functions, so they can be tested without a canvas.

import type { PlotPoint, Segment } from './protocol'

export type Side = 'external' | 'internal'

/** The tool while a cycle runs: where it is and where it has been. */
export interface LiveTool {
  position: PlotPoint
  trail: readonly PlotPoint[]
  /** Changes whenever the trail grows (the trail array itself is reused). */
  revision: number
}

type Segments = readonly Readonly<{ kind: Segment['kind']; from: PlotPoint; to: PlotPoint }>[]

function cutting(segment: Segments[number]): boolean {
  return segment.kind !== 'rapid'
}

/** External when the cuts go below the start radius (turning, external threads). */
export function cuttingSide(segments: Segments, start: PlotPoint): Side {
  const cuts = segments.filter(cutting)
  if (cuts.length === 0) {
    return 'external'
  }
  const deepest = Math.min(...cuts.map((segment) => Math.min(segment.from.x, segment.to.x)))
  return deepest < start.x ? 'external' : 'internal'
}

/**
 * The finished profile: at each of `samples` Z positions between `zFrom` and `zTo`, the deepest
 * radius any cut reaches (smallest for external, largest for internal), or null where nothing cuts.
 */
export function envelope(
  segments: Segments,
  side: Side,
  zFrom: number,
  zTo: number,
  samples: number,
): ({ z: number; x: number | null })[] {
  const cuts = segments.filter(cutting)
  const profile: { z: number; x: number | null }[] = []
  for (let index = 0; index < samples; index += 1) {
    const z = zFrom + ((zTo - zFrom) * index) / Math.max(samples - 1, 1)
    let deepest: number | null = null
    for (const segment of cuts) {
      const low = Math.min(segment.from.z, segment.to.z)
      const high = Math.max(segment.from.z, segment.to.z)
      if (z < low || z > high) {
        continue
      }
      const span = segment.to.z - segment.from.z
      const t = span === 0 ? 0 : (z - segment.from.z) / span
      const x = segment.from.x + (segment.to.x - segment.from.x) * t
      if (deepest === null || (side === 'external' ? x < deepest : x > deepest)) {
        deepest = x
      }
    }
    profile.push({ z, x: deepest })
  }
  return profile
}

/** Pass number (0-based) per segment: a pass is a run of cuts; rapids belong to the next pass. */
export function passNumbers(segments: Segments): { pass: number[]; count: number } {
  const pass: number[] = []
  let current = 0
  let cutSeen = false
  for (const segment of segments) {
    if (cutting(segment)) {
      cutSeen = true
    } else if (cutSeen) {
      current += 1
      cutSeen = false
    }
    pass.push(current)
  }
  const count = segments.some(cutting) ? (cutSeen ? current + 1 : current) : 0
  return { pass, count }
}

/** 1, 2 or 5 times a power of ten, at least `raw`. */
export function niceStep(raw: number): number {
  if (!(raw > 0) || !Number.isFinite(raw)) {
    return 1
  }
  const power = 10 ** Math.floor(Math.log10(raw))
  for (const factor of [1, 2, 5, 10]) {
    if (factor * power >= raw) {
      return factor * power
    }
  }
  return 10 * power
}

/** The closest point on a segment, its position along it (0..1) and its distance from `point`. */
export function projectOnSegment(
  point: PlotPoint,
  from: PlotPoint,
  to: PlotPoint,
): { point: PlotPoint; t: number; distance: number } {
  const dz = to.z - from.z
  const dx = to.x - from.x
  const lengthSquared = dz * dz + dx * dx
  const t =
    lengthSquared === 0 ? 0 : Math.max(0, Math.min(1, ((point.z - from.z) * dz + (point.x - from.x) * dx) / lengthSquared))
  const closest = { x: from.x + dx * t, z: from.z + dz * t }
  return { point: closest, t, distance: Math.hypot(point.z - closest.z, point.x - closest.x) }
}

function distanceToSegment(point: PlotPoint, from: PlotPoint, to: PlotPoint): number {
  return projectOnSegment(point, from, to).distance
}

/**
 * The segment the tool is on, searching forward from `previous` (the tool only moves forward
 * along the path; looking a little back absorbs noise). Passes retrace the same lines, so a
 * global nearest-segment search would jump between passes.
 */
export function segmentAt(segments: Segments, point: PlotPoint, previous: number, lookahead = 12): number {
  if (segments.length === 0) {
    return 0
  }
  const first = Math.max(0, previous - 1)
  const last = Math.min(segments.length - 1, previous + lookahead)
  let best = Math.min(Math.max(previous, 0), segments.length - 1)
  let bestDistance = Number.POSITIVE_INFINITY
  for (let index = first; index <= last; index += 1) {
    const segment = segments[index]
    if (segment === undefined) {
      continue
    }
    const distance = distanceToSegment(point, segment.from, segment.to)
    if (distance < bestDistance - 1e-9) {
      best = index
      bestDistance = distance
    }
  }
  return best
}
