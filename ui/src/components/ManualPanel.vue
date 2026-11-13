<script setup lang="ts">
import { computed, onBeforeUnmount } from 'vue'

import { settle } from '../entry'
import { panel, send } from '../machine'
import type { Direction, Feed, JogButton } from '../protocol'
import { JogHold, Touches, capturedPress } from '../jogHold'
import AppIcon from './AppIcon.vue'

const feed = computed(() => panel.status?.feed ?? 'longitudinal')
const direction = computed(() => panel.status?.direction ?? 'idle')

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

// Jog: press-and-hold with a machine-side deadman; see jogHold.ts for why the panel side checks
// the press on every refresh.
const jogHold = new JogHold(
  {
    send: (button: JogButton, held: boolean): void => {
      send({ type: 'jog', button, held })
    },
  },
  {
    setInterval: (callback: () => void, ms: number): number => window.setInterval(callback, ms),
    clearInterval: (handle: number): void => {
      window.clearInterval(handle)
    },
  },
)

const touches = new Touches()

function jogPointerDown(button: JogButton, event: PointerEvent): void {
  settle()
  // Touch is handled by the touch events; mouse events right after a touch are WebKit's emulation.
  if (event.pointerType !== 'mouse' || event.button !== 0 || touches.emulatingMouse(event.timeStamp)) {
    return
  }
  const stillHeld = capturedPress(event.currentTarget as HTMLElement, event.pointerId)
  if (stillHeld !== null) {
    jogHold.press(button, stillHeld)
  }
}

function jogTouchStart(button: JogButton, event: TouchEvent): void {
  settle()
  const touch = event.changedTouches.item(0)
  if (touch !== null) {
    jogHold.press(button, touches.start(touch.identifier, event.timeStamp))
  }
}

function touchesEnded(event: TouchEvent): void {
  for (const touch of Array.from(event.changedTouches)) {
    touches.end(touch.identifier, event.timeStamp)
  }
}

function jogTouchEnd(button: JogButton, event: TouchEvent): void {
  touchesEnded(event)
  jogHold.release(button)
}

function jogStop(button: JogButton): void {
  jogHold.release(button)
}

function stopAll(): void {
  jogHold.releaseAll()
  send({ type: 'jog_release_all' })
}

function stopTouch(event: TouchEvent): void {
  touches.touched(event.timeStamp)
  stopAll()
}

function stopPointerDown(event: PointerEvent): void {
  if (event.pointerType === 'mouse' && !touches.emulatingMouse(event.timeStamp)) {
    stopAll()
  }
}

// A touch that ends anywhere (finger slid off the button, or the end event went elsewhere) ends
// its jog: the next refresh sees the touch is no longer down.
window.addEventListener('touchend', touchesEnded, true)
window.addEventListener('touchcancel', touchesEnded, true)
onBeforeUnmount(() => {
  window.removeEventListener('touchend', touchesEnded, true)
  window.removeEventListener('touchcancel', touchesEnded, true)
})

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
        @touchstart.prevent="jogTouchStart(jog.button, $event)"
        @touchend.prevent="jogTouchEnd(jog.button, $event)"
        @touchcancel="jogTouchEnd(jog.button, $event)"
        @pointerdown="jogPointerDown(jog.button, $event)"
        @pointerup="jogStop(jog.button)"
        @pointercancel="jogStop(jog.button)"
        @lostpointercapture="jogStop(jog.button)"
        @contextmenu.prevent
      >
        <AppIcon :name="jog.icon" :size="34" />
      </button>
      <button
        class="stop-jog"
        style="grid-area: stop"
        @touchstart.prevent="stopTouch"
        @pointerdown="stopPointerDown"
      >
        STOP
      </button>
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
  /* No gestures on the jog buttons: WebKit then delivers a touch as a press at once instead of
     holding it back until the finger lifts to decide whether it was a pan. */
  touch-action: none;
}

.stop-jog {
  font-size: 1.4rem;
  background: var(--stop);
}
</style>
