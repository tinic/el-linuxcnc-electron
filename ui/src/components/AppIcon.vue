<script setup lang="ts">
// Stroke icons drawn on a 24-unit grid, so no icon font is needed.
export type IconName =
  | 'wrench'
  | 'cycle'
  | 'log'
  | 'sliders'
  | 'exit'
  | 'lock'
  | 'gear'
  | 'tool'
  | 'up'
  | 'down'
  | 'left'
  | 'right'
  | 'info'

defineProps<{ name: IconName; size?: number }>()

const paths: Record<IconName, string[]> = {
  wrench: ['M14.7 6.3a4 4 0 0 0-5.4 5.4L3 18l3 3 6.3-6.3a4 4 0 0 0 5.4-5.4l-2.5 2.5-2.4-.6-.6-2.4z'],
  cycle: ['M21 12a9 9 0 1 1-3-6.7', 'M21 4v5h-5'],
  log: ['M4 5h16', 'M4 10h16', 'M4 15h10', 'M4 20h7'],
  sliders: ['M6 4v16', 'M12 4v16', 'M18 4v16', 'M4 9h4', 'M10 15h4', 'M16 7h4'],
  exit: ['M10 4H5v16h5', 'M14 8l4 4-4 4', 'M18 12H9'],
  lock: ['M6 11h12v9H6z', 'M8.5 11V8a3.5 3.5 0 0 1 7 0v3'],
  gear: [
    'M12 15.5a3.5 3.5 0 1 0 0-7 3.5 3.5 0 0 0 0 7z',
    'M19 12l2-1.5-2-3.5-2.4.8a7 7 0 0 0-1.6-1L14.5 4h-5L9 6.8a7 7 0 0 0-1.6 1L5 7 3 10.5 5 12l-2 1.5L5 17l2.4-.8a7 7 0 0 0 1.6 1L9.5 20h5l.5-2.8a7 7 0 0 0 1.6-1L19 17l2-3.5z',
  ],
  tool: ['M3 8h18v12H3z', 'M8 8V5h8v3', 'M3 13h18', 'M10 12v2', 'M14 12v2'],
  up: ['M6 15l6-6 6 6'],
  down: ['M6 9l6 6 6-6'],
  left: ['M15 6l-6 6 6 6'],
  right: ['M9 6l6 6-6 6'],
  info: ['M12 21a9 9 0 1 0 0-18 9 9 0 0 0 0 18z', 'M12 11v6', 'M12 7.5v.5'],
}
</script>

<template>
  <svg
    :width="size ?? 22"
    :height="size ?? 22"
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    aria-hidden="true"
  >
    <path v-for="d in paths[name]" :key="d" :d="d" />
  </svg>
</template>
