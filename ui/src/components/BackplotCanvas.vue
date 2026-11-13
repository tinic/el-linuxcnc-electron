<script setup lang="ts">
// 2D tool path in the XZ plane: Z runs left (towards the chuck) to right, X (radius) upwards. The
// work is drawn as stock with the material the cycle removes hatched and the finished part below
// (above for internal cuts), on a grid labelled in the readout's units (diameter mode, mm/in).
//
// Preview: passes are shaded from the first (light) to the last (bright) and a marker runs along
// the path to show their order. Live (`live` set, while a cycle runs): passes the tool has made
// are solid, the current one is highlighted, the rest dimmed; the tool's actual track and position
// are drawn over them, and the canvas is redrawn only when the tool moves.
import { computed, onBeforeUnmount, onMounted, useTemplateRef, watch, type DeepReadonly } from 'vue'

import { cuttingSide, envelope, niceStep, passNumbers, segmentAt, type LiveTool } from '../backplot'
import { panel } from '../machine'
import type { Backplot, Segment } from '../protocol'
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

let frame = 0
let startedAt = 0
let liveSegment = 0

const side = computed(() => cuttingSide(props.plot.segments, props.start))
const passes = computed(() => passNumbers(props.plot.segments))

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

function draw(now: number): void {
  const element = canvas.value
  const context = element?.getContext('2d')
  if (!element || !context) {
    return
  }
  const colors = palette()
  const ratio = window.devicePixelRatio
  const width = element.clientWidth
  const height = element.clientHeight
  element.width = Math.round(width * ratio)
  element.height = Math.round(height * ratio)
  context.setTransform(ratio, 0, 0, ratio, 0, 0)
  context.fillStyle = colors.background
  context.fillRect(0, 0, width, height)
  context.lineJoin = 'round'
  context.lineCap = 'round'

  const live = props.live ?? null
  const segments = props.plot.segments
  const metric = panel.metric
  const diameter = panel.settings?.diameter_mode ?? false

  // Bounds: the path, the tool, and a little air around the work.
  const bounds = props.plot.bounds
  const min = { x: Math.min(bounds.min.x, live?.position.x ?? bounds.min.x), z: Math.min(bounds.min.z, live?.position.z ?? bounds.min.z) }
  const max = { x: Math.max(bounds.max.x, live?.position.x ?? bounds.max.x), z: Math.max(bounds.max.z, live?.position.z ?? bounds.max.z) }
  const padX = Math.max((max.x - min.x) * 0.15, 0.2)
  min.x -= padX
  max.x += padX * 0.4

  // Cuts are long and shallow, so X is stretched until the path fills at least `minDepthShare` of
  // the height; the factor is printed whenever it is not 1:1.
  const plotWidth = width - margin.left - margin.right
  const plotHeight = height - margin.top - margin.bottom
  const spanZ = Math.max(max.z - min.z, 1e-3)
  const spanX = Math.max(max.x - min.x, 1e-3)
  const scaleZ = Math.min(plotWidth / spanZ, plotHeight / spanX)
  const scaleX = Math.max(scaleZ, (minDepthShare * plotHeight) / spanX)
  const offsetZ = margin.left + (plotWidth - spanZ * scaleZ) / 2
  const offsetX = margin.top + (plotHeight - spanX * scaleX) / 2
  const screenZ = (z: number): number => offsetZ + (z - min.z) * scaleZ
  const screenX = (x: number): number => offsetX + (max.x - x) * scaleX
  const toScreen = (point: { x: number; z: number }): [number, number] => [screenZ(point.z), screenX(point.x)]
  const left = margin.left
  const right = width - margin.right
  const top = margin.top
  const bottom = height - margin.bottom

  // Grid, labelled in display units.
  context.font = '12px Iosevka, monospace'
  context.lineWidth = 1
  const mmPerUnit = metric ? 1 : 25.4
  const zStep = niceStep(70 / scaleZ / mmPerUnit) * mmPerUnit
  const xStep = (niceStep(40 / scaleX / mmPerUnit / (diameter ? 0.5 : 1)) * mmPerUnit) / (diameter ? 2 : 1)
  const decimals = (step: number, units: number): number => Math.max(0, -Math.floor(Math.log10(step / units) + 1e-9))
  context.strokeStyle = colors.grid
  context.fillStyle = colors.label
  context.textAlign = 'center'
  for (let z = Math.ceil(min.z / zStep) * zStep; z <= max.z + 1e-9; z += zStep) {
    const x = screenZ(z)
    if (x < left || x > right) {
      continue
    }
    context.beginPath()
    context.moveTo(x, top)
    context.lineTo(x, bottom)
    context.stroke()
    context.fillText(lengthToDisplay(z, metric).toFixed(decimals(zStep, mmPerUnit)), x, bottom + 18)
  }
  context.textAlign = 'right'
  for (let x = Math.ceil(min.x / xStep) * xStep; x <= max.x + 1e-9; x += xStep) {
    const y = screenX(x)
    if (y < top || y > bottom) {
      continue
    }
    context.beginPath()
    context.moveTo(left, y)
    context.lineTo(right, y)
    context.stroke()
    const shown = xToDisplay(x, metric, diameter)
    context.fillText(shown.toFixed(decimals(xStep * (diameter ? 2 : 1), mmPerUnit)), left - 8, y + 4)
  }
  context.textAlign = 'right'
  context.fillText(diameter ? 'Ø' : 'X', left - 8, top - 12)
  context.fillText('Z', left - 8, bottom + 18)
  context.textAlign = 'left'

  // The work: stock up to the face (Z0), the part the cycle leaves, and what it removes.
  const external = side.value === 'external'
  const workEnd = Math.min(0, max.z)
  if (workEnd > min.z) {
    const profile = envelope(segments, side.value, min.z, workEnd, profileSamples)
    const stock = props.start.x
    const edge = external ? bottom : top
    context.save()
    context.beginPath()
    context.rect(left, top, right - left, bottom - top)
    context.clip()
    // Part (material that stays).
    context.fillStyle = colors.part
    context.beginPath()
    context.moveTo(screenZ(min.z), edge)
    for (const point of profile) {
      context.lineTo(screenZ(point.z), screenX(point.x ?? stock))
    }
    context.lineTo(screenZ(workEnd), edge)
    context.closePath()
    context.fill()
    // Removed material: between the stock and the profile, hatched.
    context.beginPath()
    context.moveTo(screenZ(min.z), screenX(stock))
    for (const point of profile) {
      context.lineTo(screenZ(point.z), screenX(stock))
    }
    for (const point of [...profile].reverse()) {
      context.lineTo(screenZ(point.z), screenX(point.x ?? stock))
    }
    context.closePath()
    context.fillStyle = 'rgba(255,255,255,0.04)'
    context.fill()
    context.clip()
    context.strokeStyle = 'rgba(255,255,255,0.12)'
    for (let x = left - height; x < right; x += 9) {
      context.beginPath()
      context.moveTo(x, bottom)
      context.lineTo(x + height, top - (height - (bottom - top)))
      context.stroke()
    }
    context.restore()
    // Stock surface.
    context.strokeStyle = colors.text
    context.setLineDash([4, 4])
    context.beginPath()
    context.moveTo(screenZ(min.z), screenX(stock))
    context.lineTo(screenZ(workEnd), screenX(stock))
    context.stroke()
    context.setLineDash([])
  }

  // Work face (Z0).
  const faceZ = screenZ(0)
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

  // The path. In live mode, find where the tool is along it.
  if (live) {
    liveSegment = segmentAt(segments, live.position, liveSegment)
  }
  const currentPass = live ? (passes.value.pass[liveSegment] ?? 0) : -1
  const total = segments.reduce((sum, segment) => sum + lengthOf(segment), 0)
  const progress = (((now - startedAt) / 1000) % lapSeconds) / lapSeconds
  let remaining = progress * total
  let marker = toScreen(props.start)
  const passCount = Math.max(passes.value.count, 1)

  segments.forEach((segment, index) => {
    const [x1, y1] = toScreen(segment.from)
    const [x2, y2] = toScreen(segment.to)
    const pass = passes.value.pass[index] ?? 0
    const rapid = segment.kind === 'rapid'
    let alpha: number
    let lineWidth = rapid ? 1 : 2
    if (live) {
      alpha = pass < currentPass ? 0.85 : pass === currentPass ? 1 : 0.22
      if (pass === currentPass && !rapid) {
        lineWidth = 3.5
      }
    } else {
      // Later (deeper) passes brighter, so the finishing pass stands out.
      alpha = rapid ? 0.5 : 0.35 + (0.65 * (pass + 1)) / passCount
      if (remaining > 0) {
        const length = lengthOf(segment)
        const t = length > 0 ? Math.min(remaining / length, 1) : 1
        marker = [x1 + (x2 - x1) * t, y1 + (y2 - y1) * t]
      }
      remaining -= lengthOf(segment)
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

  if (live) {
    // Where the tool has actually been during this cycle.
    context.strokeStyle = colors.track
    context.lineWidth = 2
    context.beginPath()
    live.trail.forEach((point, index) => {
      const [x, y] = toScreen(point)
      if (index === 0) {
        context.moveTo(x, y)
      } else {
        context.lineTo(x, y)
      }
    })
    context.stroke()
    marker = toScreen(live.position)
  }
  context.fillStyle = colors.tool
  context.beginPath()
  context.arc(marker[0], marker[1], live ? 7 : 5.5, 0, 2 * Math.PI)
  context.fill()
  if (live) {
    context.strokeStyle = colors.tool
    context.lineWidth = 1.5
    context.beginPath()
    context.arc(marker[0], marker[1], 12, 0, 2 * Math.PI)
    context.stroke()
  }

  // Legend and scale note.
  context.font = '12px Iosevka, monospace'
  context.textAlign = 'left'
  const legend: [string, string, boolean][] = [
    ['rapid', colors.rapid, true],
    [segments.some((segment) => segment.kind === 'thread') ? 'thread' : 'feed', segments.some((segment) => segment.kind === 'thread') ? colors.thread : colors.feed, false],
  ]
  let legendX = left + 6
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
  if (live) {
    context.fillStyle = colors.text
    context.fillText(`pass ${String(Math.min(currentPass + 1, passCount))} of ${String(passCount)}`, legendX, 18)
  }
  const stretch = scaleX / scaleZ
  context.textAlign = 'right'
  context.fillStyle = colors.label
  context.fillText(stretch > 1.05 ? `X ×${stretch.toFixed(1)}` : 'X 1:1', right, 18)

  if (!live) {
    frame = requestAnimationFrame(draw)
  }
}

function restart(): void {
  cancelAnimationFrame(frame)
  startedAt = performance.now()
  liveSegment = 0
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
