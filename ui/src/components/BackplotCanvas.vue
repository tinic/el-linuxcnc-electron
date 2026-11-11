<script setup lang="ts">
// 2D tool path in the XZ plane: Z runs left (towards the chuck) to right, X (radius) upwards.
// A marker follows the path so the order of passes is visible.
import { onBeforeUnmount, onMounted, useTemplateRef, watch, type DeepReadonly } from 'vue'

import type { Backplot, Segment } from '../protocol'

const props = defineProps<{ plot: DeepReadonly<Backplot>; start: { x: number; z: number } }>()

const canvas = useTemplateRef<HTMLCanvasElement>('canvas')
const marginPx = 36
const lapSeconds = 6
const minDepthShare = 0.6

const colors: Record<Segment['kind'], string> = {
  rapid: '#7a7a7a',
  feed: '#3b82f6',
  thread: '#f59e0b',
}

let frame = 0
let startedAt = 0

function lengthOf(segment: DeepReadonly<Segment>): number {
  return Math.hypot(segment.to.x - segment.from.x, segment.to.z - segment.from.z)
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
  element.width = Math.round(width * ratio)
  element.height = Math.round(height * ratio)
  context.setTransform(ratio, 0, 0, ratio, 0, 0)
  context.clearRect(0, 0, width, height)

  // Cuts are long and shallow, so X is stretched until the path fills at least
  // `minDepthShare` of the height; the factor is printed whenever it is not 1:1.
  const { min, max } = props.plot.bounds
  const spanZ = Math.max(max.z - min.z, 1e-3)
  const spanX = Math.max(max.x - min.x, 1e-3)
  const scaleZ = Math.min((width - 2 * marginPx) / spanZ, (height - 2 * marginPx) / spanX)
  const scaleX = Math.max(scaleZ, (minDepthShare * (height - 2 * marginPx)) / spanX)
  const offsetZ = (width - spanZ * scaleZ) / 2
  const offsetX = (height - spanX * scaleX) / 2
  const toScreen = (point: { x: number; z: number }): [number, number] => [
    offsetZ + (point.z - min.z) * scaleZ,
    height - offsetX - (point.x - min.x) * scaleX,
  ]
  const stretch = scaleX / scaleZ
  context.fillStyle = 'rgba(255,255,255,0.45)'
  context.font = '14px Iosevka, monospace'
  context.fillText(stretch > 1.05 ? `X ×${stretch.toFixed(1)}` : 'X 1:1', 12, 22)

  // Work face (Z0) and the start position.
  context.strokeStyle = 'rgba(255,255,255,0.15)'
  context.setLineDash([4, 6])
  const [faceZ] = toScreen({ x: 0, z: 0 })
  context.beginPath()
  context.moveTo(faceZ, 0)
  context.lineTo(faceZ, height)
  context.stroke()

  const total = props.plot.segments.reduce((sum, segment) => sum + lengthOf(segment), 0)
  const progress = (((now - startedAt) / 1000) % lapSeconds) / lapSeconds
  let remaining = progress * total
  let marker = toScreen(props.start)

  context.lineWidth = 2
  for (const segment of props.plot.segments) {
    const [x1, y1] = toScreen(segment.from)
    const [x2, y2] = toScreen(segment.to)
    context.setLineDash(segment.kind === 'rapid' ? [6, 5] : [])
    context.strokeStyle = colors[segment.kind]
    context.globalAlpha = remaining > 0 ? 1 : 0.35
    context.beginPath()
    context.moveTo(x1, y1)
    context.lineTo(x2, y2)
    context.stroke()
    const length = lengthOf(segment)
    if (remaining > 0) {
      const t = length > 0 ? Math.min(remaining / length, 1) : 1
      marker = [x1 + (x2 - x1) * t, y1 + (y2 - y1) * t]
    }
    remaining -= length
  }
  context.globalAlpha = 1
  context.setLineDash([])
  context.fillStyle = '#ef4444'
  context.beginPath()
  context.arc(marker[0], marker[1], 6, 0, 2 * Math.PI)
  context.fill()

  frame = requestAnimationFrame(draw)
}

function restart(): void {
  cancelAnimationFrame(frame)
  startedAt = performance.now()
  frame = requestAnimationFrame(draw)
}

onMounted(restart)
watch(() => props.plot, restart)
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
