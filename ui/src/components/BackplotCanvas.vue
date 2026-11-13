<script setup lang="ts">
// 2D tool path in the XZ plane: Z runs left (towards the chuck) to right, X (radius) upwards. The
// work is drawn as stock with the material the cycle removes hatched and the finished part below
// (above for internal cuts), on a grid labelled in the readout's units (diameter mode, mm/in).
//
// Preview: passes are shaded from the first (light) to the last (bright) and a marker runs along
// the path to show their order. Live (`live` set, while a cycle runs): passes the tool has made
// are solid, the current one is highlighted, the rest dimmed; the tool's actual track and position
// are drawn over them.
//
// Rendering is layered so it stays cheap on the lathe's PC: everything but the tool is drawn once
// into a static layer, rebuilt only when the size, units, view or current pass change; the track
// is appended point by point to its own layer; a frame then copies the two layers and draws the
// tool marker.
import { computed, onBeforeUnmount, onMounted, useTemplateRef, watch, type DeepReadonly } from 'vue'

import { cuttingSide, envelope, niceStep, passNumbers, segmentAt, type LiveTool } from '../backplot'
import { panel } from '../machine'
import type { Backplot, PlotPoint, Segment } from '../protocol'
import { lengthToDisplay, xToDisplay } from '../units'

const props = defineProps<{
  plot: DeepReadonly<Backplot>
  start: { x: number; z: number }
  live?: DeepReadonly<LiveTool> | null
}>()

const canvas = useTemplateRef<HTMLCanvasElement>('canvas')
const margin = { left: 56, right: 16, top: 34, bottom: 30 }
const lapSeconds = 6
const minDepthShare = 0.6
const profileSamples = 300

interface Palette {
  background: string
  part: string
  grid: string
  label: string
  text: string
  track: string
  feed: string
  thread: string
  rapid: string
  tool: string
}

interface Bounds {
  min: PlotPoint
  max: PlotPoint
}

/** Plot coordinates to CSS pixels. */
interface View {
  width: number
  height: number
  ratio: number
  min: PlotPoint
  max: PlotPoint
  scaleZ: number
  scaleX: number
  offsetZ: number
  offsetX: number
}

let frame = 0
let startedAt = 0
let liveSegment = 0
let staticLayer: HTMLCanvasElement | null = null
let staticKey = ''
let trailLayer: HTMLCanvasElement | null = null
let trailDrawn = 0
let trailFirst: DeepReadonly<PlotPoint> | undefined
let viewBounds: Bounds | null = null
/** Theme colours, read when the static layer is rebuilt (getComputedStyle is not cheap). */
let colors: Palette | null = null

const side = computed(() => cuttingSide(props.plot.segments, props.start))
const passes = computed(() => passNumbers(props.plot.segments))
/** Cumulative path length at the end of each segment, for the preview marker. */
const cumulative = computed(() => {
  let total = 0
  return props.plot.segments.map((segment) => (total += lengthOf(segment)))
})

function palette(): Palette {
  const style = getComputedStyle(document.documentElement)
  const value = (name: string, fallback: string): string => style.getPropertyValue(name).trim() || fallback
  return {
    background: value('--bg-sunken', '#161616'),
    part: value('--control-active', '#4a4a4a'),
    grid: 'rgba(255,255,255,0.07)',
    label: value('--text-faint', '#5c5c5c'),
    text: value('--text-dim', '#9a9a9a'),
    track: value('--text', '#f2f2f2'),
    feed: value('--accent', '#3b82f6'),
    thread: value('--warn', '#f59e0b'),
    rapid: '#7a7a7a',
    tool: value('--entry', '#ef4444'),
  }
}

function lengthOf(segment: DeepReadonly<Segment>): number {
  return Math.hypot(segment.to.x - segment.from.x, segment.to.z - segment.from.z)
}

/** The path's bounds with air around the work; while live, grown (with margin) to keep the tool in view. */
function boundsFor(live: DeepReadonly<LiveTool> | null): Bounds {
  const base = props.plot.bounds
  if (viewBounds === null) {
    const padX = Math.max((base.max.x - base.min.x) * 0.15, 0.2)
    viewBounds = { min: { x: base.min.x - padX, z: base.min.z }, max: { x: base.max.x + padX * 0.4, z: base.max.z } }
  }
  if (live) {
    const { x, z } = live.position
    const view = viewBounds
    if (x < view.min.x || x > view.max.x || z < view.min.z || z > view.max.z) {
      const padZ = Math.max((view.max.z - view.min.z) * 0.1, 1)
      const padX = Math.max((view.max.x - view.min.x) * 0.1, 0.5)
      viewBounds = {
        min: { x: Math.min(view.min.x, x - padX), z: Math.min(view.min.z, z - padZ) },
        max: { x: Math.max(view.max.x, x + padX), z: Math.max(view.max.z, z + padZ) },
      }
    }
  }
  return viewBounds
}

function viewFor(width: number, height: number, ratio: number, bounds: Bounds): View {
  // Cuts are long and shallow, so X is stretched until the path fills at least `minDepthShare` of
  // the height; the factor is printed whenever it is not 1:1.
  const plotWidth = width - margin.left - margin.right
  const plotHeight = height - margin.top - margin.bottom
  const spanZ = Math.max(bounds.max.z - bounds.min.z, 1e-3)
  const spanX = Math.max(bounds.max.x - bounds.min.x, 1e-3)
  const scaleZ = Math.min(plotWidth / spanZ, plotHeight / spanX)
  const scaleX = Math.max(scaleZ, (minDepthShare * plotHeight) / spanX)
  return {
    width,
    height,
    ratio,
    min: bounds.min,
    max: bounds.max,
    scaleZ,
    scaleX,
    offsetZ: margin.left + (plotWidth - spanZ * scaleZ) / 2,
    offsetX: margin.top + (plotHeight - spanX * scaleX) / 2,
  }
}

function screen(view: View, point: { x: number; z: number }): [number, number] {
  return [view.offsetZ + (point.z - view.min.z) * view.scaleZ, view.offsetX + (view.max.x - point.x) * view.scaleX]
}

function layer(existing: HTMLCanvasElement | null, view: View): [HTMLCanvasElement, CanvasRenderingContext2D] | null {
  const target = existing ?? document.createElement('canvas')
  target.width = Math.round(view.width * view.ratio)
  target.height = Math.round(view.height * view.ratio)
  const context = target.getContext('2d')
  if (!context) {
    return null
  }
  context.setTransform(view.ratio, 0, 0, view.ratio, 0, 0)
  context.lineJoin = 'round'
  context.lineCap = 'round'
  return [target, context]
}

function drawGrid(context: CanvasRenderingContext2D, view: View, colors: Palette): void {
  const metric = panel.metric
  const diameter = panel.settings?.diameter_mode ?? false
  const left = margin.left
  const right = view.width - margin.right
  const top = margin.top
  const bottom = view.height - margin.bottom
  context.font = '12px Iosevka, monospace'
  context.lineWidth = 1
  const mmPerUnit = metric ? 1 : 25.4
  const zStep = niceStep(70 / view.scaleZ / mmPerUnit) * mmPerUnit
  const xStep = (niceStep(40 / view.scaleX / mmPerUnit / (diameter ? 0.5 : 1)) * mmPerUnit) / (diameter ? 2 : 1)
  const decimals = (step: number): number => Math.max(0, -Math.floor(Math.log10(step / mmPerUnit) + 1e-9))
  context.strokeStyle = colors.grid
  context.fillStyle = colors.label
  context.textAlign = 'center'
  for (let z = Math.ceil(view.min.z / zStep) * zStep; z <= view.max.z + 1e-9; z += zStep) {
    const [x] = screen(view, { x: 0, z })
    if (x < left || x > right) {
      continue
    }
    context.beginPath()
    context.moveTo(x, top)
    context.lineTo(x, bottom)
    context.stroke()
    if (x > left + 14) {
      // (a label at the very edge would run into the axis name)
      context.fillText(lengthToDisplay(z, metric).toFixed(decimals(zStep)), x, bottom + 18)
    }
  }
  context.textAlign = 'right'
  for (let x = Math.ceil(view.min.x / xStep) * xStep; x <= view.max.x + 1e-9; x += xStep) {
    const [, y] = screen(view, { x, z: 0 })
    if (y < top || y > bottom) {
      continue
    }
    context.beginPath()
    context.moveTo(left, y)
    context.lineTo(right, y)
    context.stroke()
    context.fillText(xToDisplay(x, metric, diameter).toFixed(decimals(xStep * (diameter ? 2 : 1))), left - 8, y + 4)
  }
  context.fillText(diameter ? 'Ø' : 'X', left - 8, top - 12)
  context.fillText('Z', left - 8, bottom + 18)
}

function drawWork(context: CanvasRenderingContext2D, view: View, colors: Palette): void {
  const left = margin.left
  const right = view.width - margin.right
  const top = margin.top
  const bottom = view.height - margin.bottom
  const external = side.value === 'external'
  const workEnd = Math.min(0, view.max.z)
  if (workEnd > view.min.z) {
    const profile = envelope(props.plot.segments, side.value, view.min.z, workEnd, profileSamples)
    const stock = props.start.x
    const edge = external ? bottom : top
    const sz = (z: number): number => screen(view, { x: 0, z })[0]
    const sx = (x: number): number => screen(view, { x, z: 0 })[1]
    context.save()
    context.beginPath()
    context.rect(left, top, right - left, bottom - top)
    context.clip()
    // Part (material that stays).
    context.fillStyle = colors.part
    context.beginPath()
    context.moveTo(sz(view.min.z), edge)
    for (const point of profile) {
      context.lineTo(sz(point.z), sx(point.x ?? stock))
    }
    context.lineTo(sz(workEnd), edge)
    context.closePath()
    context.fill()
    // Removed material: between the stock and the profile, hatched.
    context.beginPath()
    context.moveTo(sz(view.min.z), sx(stock))
    for (const point of profile) {
      context.lineTo(sz(point.z), sx(stock))
    }
    for (const point of [...profile].reverse()) {
      context.lineTo(sz(point.z), sx(point.x ?? stock))
    }
    context.closePath()
    context.fillStyle = 'rgba(255,255,255,0.04)'
    context.fill()
    context.clip()
    context.strokeStyle = 'rgba(255,255,255,0.12)'
    context.lineWidth = 1
    for (let x = left - view.height; x < right; x += 9) {
      context.beginPath()
      context.moveTo(x, bottom)
      context.lineTo(x + view.height, bottom - view.height)
      context.stroke()
    }
    context.restore()
    // Stock surface.
    context.strokeStyle = colors.text
    context.setLineDash([4, 4])
    context.beginPath()
    context.moveTo(sz(view.min.z), sx(stock))
    context.lineTo(sz(workEnd), sx(stock))
    context.stroke()
    context.setLineDash([])
  }
  // Work face (Z0).
  const [faceZ] = screen(view, { x: 0, z: 0 })
  if (faceZ >= left && faceZ <= right) {
    context.strokeStyle = colors.text
    context.lineWidth = 1
    context.setLineDash([2, 4])
    context.beginPath()
    context.moveTo(faceZ, top - 8)
    context.lineTo(faceZ, bottom)
    context.stroke()
    context.setLineDash([])
    context.fillStyle = colors.text
    context.textAlign = 'center'
    context.fillText('Z0', faceZ, top - 12)
  }
}

function drawPath(context: CanvasRenderingContext2D, view: View, colors: Palette, currentPass: number): void {
  const passCount = Math.max(passes.value.count, 1)
  props.plot.segments.forEach((segment, index) => {
    const [x1, y1] = screen(view, segment.from)
    const [x2, y2] = screen(view, segment.to)
    const pass = passes.value.pass[index] ?? 0
    const rapid = segment.kind === 'rapid'
    let alpha: number
    let lineWidth = rapid ? 1 : 2
    if (currentPass >= 0) {
      alpha = pass < currentPass ? 0.85 : pass === currentPass ? 1 : 0.22
      if (pass === currentPass && !rapid) {
        lineWidth = 3.5
      }
    } else {
      // Later (deeper) passes brighter, so the finishing pass stands out.
      alpha = rapid ? 0.5 : 0.35 + (0.65 * (pass + 1)) / passCount
    }
    context.globalAlpha = alpha
    context.lineWidth = lineWidth
    context.setLineDash(rapid ? [5, 5] : [])
    context.strokeStyle = rapid ? colors.rapid : segment.kind === 'thread' ? colors.thread : colors.feed
    context.beginPath()
    context.moveTo(x1, y1)
    context.lineTo(x2, y2)
    context.stroke()
    // Direction of cutting moves.
    const pixels = Math.hypot(x2 - x1, y2 - y1)
    if (!rapid && pixels > 40) {
      const angle = Math.atan2(y2 - y1, x2 - x1)
      const [mx, my] = [(x1 + x2) / 2, (y1 + y2) / 2]
      context.setLineDash([])
      context.beginPath()
      context.moveTo(mx - 7 * Math.cos(angle - 0.45), my - 7 * Math.sin(angle - 0.45))
      context.lineTo(mx, my)
      context.lineTo(mx - 7 * Math.cos(angle + 0.45), my - 7 * Math.sin(angle + 0.45))
      context.stroke()
    }
  })
  context.globalAlpha = 1
  context.setLineDash([])
}

function drawLegend(context: CanvasRenderingContext2D, view: View, colors: Palette, currentPass: number): void {
  const segments = props.plot.segments
  context.font = '12px Iosevka, monospace'
  context.textAlign = 'left'
  // Both cycles cut with spindle-synchronized moves (G33); "cut" fits turning and threading.
  const legend: [string, string, boolean][] = [
    ['rapid', colors.rapid, true],
    ['cut', segments.some((segment) => segment.kind === 'thread') ? colors.thread : colors.feed, false],
  ]
  let legendX = margin.left + 6
  for (const [name, color, dashed] of legend) {
    context.strokeStyle = color
    context.lineWidth = 2
    context.setLineDash(dashed ? [5, 5] : [])
    context.beginPath()
    context.moveTo(legendX, 14)
    context.lineTo(legendX + 22, 14)
    context.stroke()
    context.fillStyle = colors.text
    context.fillText(name, legendX + 28, 18)
    legendX += 28 + context.measureText(name).width + 18
  }
  context.setLineDash([])
  if (currentPass >= 0) {
    const passCount = Math.max(passes.value.count, 1)
    context.fillStyle = colors.text
    context.fillText(`pass ${String(Math.min(currentPass + 1, passCount))} of ${String(passCount)}`, legendX, 18)
  }
  const stretch = view.scaleX / view.scaleZ
  context.textAlign = 'right'
  context.fillStyle = colors.label
  context.fillText(stretch > 1.05 ? `X ×${stretch.toFixed(1)}` : 'X 1:1', view.width - margin.right, 18)
}

function rebuildStatic(view: View, currentPass: number): void {
  const made = layer(staticLayer, view)
  if (!made) {
    return
  }
  const [target, context] = made
  staticLayer = target
  colors = palette()
  context.fillStyle = colors.background
  context.fillRect(0, 0, view.width, view.height)
  drawGrid(context, view, colors)
  drawWork(context, view, colors)
  drawPath(context, view, colors, currentPass)
  drawLegend(context, view, colors, currentPass)
  // The track is in the same coordinates; start it again.
  trailLayer = layer(trailLayer, view)?.[0] ?? null
  trailDrawn = 0
}

/** Appends the track points not drawn yet (all of them after a rebuild or a restart). */
function extendTrail(view: View, trail: DeepReadonly<PlotPoint[]>): void {
  const context = trailLayer?.getContext('2d')
  if (!trailLayer || !context) {
    return
  }
  if (trail.length < trailDrawn || trail[0] !== trailFirst) {
    context.clearRect(0, 0, view.width, view.height)
    trailDrawn = 0
  }
  trailFirst = trail[0]
  if (trail.length - trailDrawn < 1) {
    return
  }
  context.strokeStyle = (colors ?? palette()).track
  context.lineWidth = 2
  context.beginPath()
  const from = Math.max(trailDrawn - 1, 0)
  trail.slice(from).forEach((point, index) => {
    const [x, y] = screen(view, point)
    if (index === 0) {
      context.moveTo(x, y)
    } else {
      context.lineTo(x, y)
    }
  })
  context.stroke()
  trailDrawn = trail.length
}

/** Where the preview marker is after `seconds`: it laps the path every `lapSeconds`. */
function previewMarker(seconds: number): PlotPoint {
  const lengths = cumulative.value
  const total = lengths.at(-1) ?? 0
  const travelled = ((seconds % lapSeconds) / lapSeconds) * total
  const index = lengths.findIndex((end) => end >= travelled)
  const segment = props.plot.segments[index]
  if (!segment) {
    return props.start
  }
  const startsAt = index > 0 ? (lengths[index - 1] ?? 0) : 0
  const length = lengthOf(segment)
  const t = length > 0 ? (travelled - startsAt) / length : 1
  return { x: segment.from.x + (segment.to.x - segment.from.x) * t, z: segment.from.z + (segment.to.z - segment.from.z) * t }
}

function draw(now: number): void {
  const element = canvas.value
  const context = element?.getContext('2d')
  if (!element || !context) {
    return
  }
  const ratio = window.devicePixelRatio
  const width = element.clientWidth
  const height = element.clientHeight
  const pixelWidth = Math.round(width * ratio)
  const pixelHeight = Math.round(height * ratio)
  if (element.width !== pixelWidth || element.height !== pixelHeight) {
    element.width = pixelWidth
    element.height = pixelHeight
  }

  const live = props.live ?? null
  const bounds = boundsFor(live)
  const view = viewFor(width, height, ratio, bounds)
  if (live) {
    liveSegment = segmentAt(props.plot.segments, live.position, liveSegment)
  }
  const currentPass = live ? (passes.value.pass[liveSegment] ?? 0) : -1
  const key = [
    width,
    height,
    ratio,
    panel.metric,
    panel.settings?.diameter_mode ?? false,
    currentPass,
    bounds.min.x,
    bounds.min.z,
    bounds.max.x,
    bounds.max.z,
  ].join('|')
  if (key !== staticKey || !staticLayer) {
    rebuildStatic(view, currentPass)
    staticKey = key
  }

  context.setTransform(1, 0, 0, 1, 0, 0)
  if (staticLayer) {
    context.drawImage(staticLayer, 0, 0)
  }
  let marker: PlotPoint
  if (live) {
    extendTrail(view, live.trail)
    if (trailLayer) {
      context.drawImage(trailLayer, 0, 0)
    }
    marker = live.position
  } else {
    marker = previewMarker((now - startedAt) / 1000)
  }
  context.setTransform(ratio, 0, 0, ratio, 0, 0)
  const [mx, my] = screen(view, marker)
  context.fillStyle = (colors ?? palette()).tool
  context.beginPath()
  context.arc(mx, my, live ? 7 : 5.5, 0, 2 * Math.PI)
  context.fill()
  if (live) {
    context.strokeStyle = context.fillStyle
    context.lineWidth = 1.5
    context.beginPath()
    context.arc(mx, my, 12, 0, 2 * Math.PI)
    context.stroke()
  }

  if (!live) {
    frame = requestAnimationFrame(draw)
  }
}

function restart(): void {
  cancelAnimationFrame(frame)
  startedAt = performance.now()
  liveSegment = 0
  viewBounds = null
  staticKey = ''
  trailDrawn = 0
  frame = requestAnimationFrame(draw)
}

onMounted(restart)
watch(() => props.plot, restart)
// A new `live` object arrives with every tool update; the trail itself is not watched deeply.
watch(
  () => props.live,
  () => {
    cancelAnimationFrame(frame)
    frame = requestAnimationFrame(draw)
  },
)
onBeforeUnmount(() => {
  cancelAnimationFrame(frame)
})
</script>

<template>
  <canvas ref="canvas" class="plot"></canvas>
</template>

<style scoped>
.plot {
  width: 100%;
  height: 100%;
  display: block;
  border-radius: var(--radius);
  background: var(--bg-sunken);
}
</style>
