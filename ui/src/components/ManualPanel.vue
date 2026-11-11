<script setup lang="ts">
import { computed, onBeforeUnmount } from 'vue'

import { settle } from '../entry'
import { panel, send } from '../machine'
import type { Direction, Feed, JogButton } from '../protocol'
import AppIcon from './AppIcon.vue'

const feed = computed(() => panel.status?.feed ?? 'longitudinal')
const direction = computed(() => panel.status?.direction ?? 'forward')

const feeds: { value: Feed; label: string; glyph: string }[] = [
  { value: 'longitudinal', label: 'Longitudinal', glyph: '⬌' },
  { value: 'cross', label: 'Cross', glyph: '⬍' },
  { value: 'front_compound', label: 'Front Compound', glyph: '⬋' },
  { value: 'back_compound', label: 'Back Compound', glyph: '⬉' },
]

// Which way "forward" and "reverse" move the tool for the selected feed, as in the original.
const forwardGlyph: Record<Feed, string> = {
  longitudinal: '⬅',
  cross: '⬆',
  front_compound: '⬋',
  back_compound: '⬉',
}
const reverseGlyph: Record<Feed, string> = {
  longitudinal: '⮕',
  cross: '⬇',
  front_compound: '⬈',
  back_compound: '⬊',
}

const directions = computed<{ value: Direction; label: string; glyph: string }[]>(() => [
  { value: 'forward', label: 'Forward', glyph: forwardGlyph[feed.value] },
  { value: 'reverse', label: 'Reverse', glyph: reverseGlyph[feed.value] },
  { value: 'hold', label: 'Hold', glyph: '⏸' },
  { value: 'idle', label: 'Idle', glyph: '⏹' },
])

function chooseFeed(value: Feed): void {
  settle()
  send({ type: 'set_feed', feed: value })
}

function chooseDirection(value: Direction): void {
  settle()
  send({ type: 'set_direction', direction: value })
}

// Jog: while a button is held the panel re-sends "held" every 50 ms. The machine stops the slide
// by itself if those refreshes stop for 150 ms (frozen panel, lost touch release).
const refreshMs = 50
const timers = new Map<JogButton, number>()

function jogStart(button: JogButton, event: PointerEvent): void {
  settle()
  ;(event.currentTarget as HTMLElement).setPointerCapture(event.pointerId)
  jogStop(button)
  send({ type: 'jog', button, held: true })
  timers.set(
    button,
    window.setInterval(() => {
      send({ type: 'jog', button, held: true })
    }, refreshMs),
  )
}

function jogStop(button: JogButton): void {
  const timer = timers.get(button)
  if (timer !== undefined) {
    window.clearInterval(timer)
    timers.delete(button)
    send({ type: 'jog', button, held: false })
  }
}

function stopAll(): void {
  for (const button of [...timers.keys()]) {
    jogStop(button)
  }
  send({ type: 'jog_release_all' })
}

onBeforeUnmount(stopAll)

const jogs: { button: JogButton; icon: 'up' | 'down' | 'left' | 'right'; area: string }[] = [
  { button: 'x_minus', icon: 'up', area: 'up' },
  { button: 'z_minus', icon: 'left', area: 'left' },
  { button: 'z_plus', icon: 'right', area: 'right' },
  { button: 'x_plus', icon: 'down', area: 'down' },
]
</script>

<template>
  <div class="manual">
    <section>
      <h2 class="section-title">Feed</h2>
      <button
        v-for="option in feeds"
        :key="option.value"
        class="option"
        :class="{ selected: feed === option.value }"
        @click="chooseFeed(option.value)"
      >
        <span class="dot"></span><span class="glyph">{{ option.glyph }}</span>{{ option.label }}
      </button>
    </section>
    <section>
      <h2 class="section-title">Direction</h2>
      <button
        v-for="option in directions"
        :key="option.value"
        class="option"
        :class="{ selected: direction === option.value }"
        @click="chooseDirection(option.value)"
      >
        <span class="dot"></span><span class="glyph">{{ option.glyph }}</span>{{ option.label }}
      </button>
    </section>
    <section class="jog">
      <button
        v-for="jog in jogs"
        :key="jog.button"
        :style="{ gridArea: jog.area }"
        @pointerdown="jogStart(jog.button, $event)"
        @pointerup="jogStop(jog.button)"
        @pointercancel="jogStop(jog.button)"
        @lostpointercapture="jogStop(jog.button)"
        @contextmenu.prevent
      >
        <AppIcon :name="jog.icon" :size="34" />
      </button>
      <button class="stop-jog" style="grid-area: stop" @pointerdown="stopAll">STOP</button>
    </section>
  </div>
</template>

<style scoped>
.manual {
  display: grid;
  grid-template-columns: 250px 230px 1fr;
  gap: 28px;
  align-items: start;
}

section {
  display: grid;
  gap: var(--gap);
}

.glyph {
  width: 1.2em;
  text-align: center;
}

.jog {
  grid-template-columns: repeat(3, 120px);
  grid-template-rows: repeat(3, 96px);
  grid-template-areas:
    '. up .'
    'left stop right'
    '. down .';
  justify-content: center;
  margin-top: 4px;
}

.jog button {
  display: grid;
  place-items: center;
}

.stop-jog {
  font-size: 1.4rem;
  background: var(--stop);
}
</style>
