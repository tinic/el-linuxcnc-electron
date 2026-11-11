<script setup lang="ts">
import { computed } from 'vue'

import { settle } from '../entry'
import { panel, send, showPage, type Page } from '../machine'
import AppIcon, { type IconName } from './AppIcon.vue'

const items: { page: Page; label: string; icon: IconName }[] = [
  { page: 'manual', label: 'Manual', icon: 'wrench' },
  { page: 'cycles', label: 'Canned Cycles', icon: 'cycle' },
  { page: 'log', label: 'LinuxCNC', icon: 'log' },
  { page: 'settings', label: 'Settings', icon: 'sliders' },
]

type Light = 'red' | 'amber' | 'green' | 'white' | 'blue'

// Industrial stack light: red error, amber disconnected, green cycle running, white canned-cycle
// mode, blue manual mode.
const status = computed<{ light: Light | null; text: string }>(() => {
  const machine = panel.status
  if (!panel.connected || !machine) {
    return { light: 'amber', text: 'CONNECTING' }
  }
  if (!machine.task.connected) {
    return { light: 'amber', text: 'NO LINUXCNC' }
  }
  if (machine.task.error) {
    return { light: 'red', text: 'ERROR' }
  }
  if (machine.task.program_running) {
    return { light: 'green', text: 'RUNNING' }
  }
  if (machine.mode === 'cycle') {
    return { light: 'white', text: 'CANNED CYCLE' }
  }
  if (machine.mode === 'manual') {
    return { light: 'blue', text: 'MANUAL' }
  }
  return { light: null, text: 'IDLE' }
})

const lights: Light[] = ['red', 'amber', 'green', 'white', 'blue']

function go(page: Page): void {
  settle()
  showPage(page)
}

function quit(): void {
  settle()
  send({ type: 'quit' })
}
</script>

<template>
  <aside class="sidebar">
    <div class="brand"><span class="mark"></span>ELLE</div>
    <nav>
      <button
        v-for="item in items"
        :key="item.page"
        class="item"
        :class="{ active: panel.page === item.page }"
        @click="go(item.page)"
      >
        <AppIcon :name="item.icon" :size="24" />
        <span>{{ item.label }}</span>
      </button>
    </nav>
    <div class="status" :title="status.text">
      <div class="stack">
        <div class="cap"></div>
        <div v-for="light in lights" :key="light" class="lamp" :class="[light, { on: status.light === light }]"></div>
      </div>
      <div class="text">
        <span class="caption">Status</span>
        <span class="value" :class="status.light">{{ status.text }}</span>
        <span v-if="panel.simulated" class="caption sim">Simulator</span>
      </div>
    </div>
    <button class="item exit" @click="quit">
      <AppIcon name="exit" :size="24" />
      <span>Exit</span>
    </button>
  </aside>
</template>

<style scoped>
.sidebar {
  display: flex;
  flex-direction: column;
  width: 200px;
  height: 100%;
  background: #181818;
  border-right: 1px solid var(--line);
}

.brand {
  display: flex;
  align-items: center;
  gap: 12px;
  height: 64px;
  padding: 0 20px;
  border-bottom: 1px solid var(--line);
  font-size: 1.5rem;
  letter-spacing: 0.08em;
}

.mark {
  width: 10px;
  height: 22px;
  border-radius: 2px;
  background: var(--accent);
}

nav {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 12px 8px;
}

.item {
  display: flex;
  align-items: center;
  gap: 14px;
  height: 60px;
  padding: 0 14px;
  border-left: 4px solid transparent;
  background: transparent;
  color: var(--text-dim);
  font-size: 1.05rem;
  text-align: left;
  white-space: nowrap;
}

.item.active {
  background: var(--control);
  border-left-color: var(--accent);
  color: var(--text);
}

.status {
  display: flex;
  align-items: center;
  gap: 16px;
  margin: auto 8px 12px;
  padding: 12px 16px 14px;
  border: 1px solid var(--line);
  border-radius: var(--radius);
  background: #1f1f1f;
}

.stack {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 5px;
}

.cap {
  width: 34px;
  height: 8px;
  border-radius: 4px 4px 0 0;
  background: #444;
}

.lamp {
  width: 28px;
  height: 16px;
  border-radius: 50%;
  border: 3px solid #333;
  transition: background 0.2s, box-shadow 0.2s;
}

.lamp.red { background: #441a1a; border-color: #662222; }
.lamp.amber { background: #442a0a; border-color: #664411; }
.lamp.green { background: #1a441a; border-color: #226622; }
.lamp.white { background: #444; border-color: #666; }
.lamp.blue { background: #1a2244; border-color: #223366; }
.lamp.red.on { background: #ef4444; box-shadow: 0 0 20px #ef4444; }
.lamp.amber.on { background: #f59e0b; box-shadow: 0 0 20px #f59e0b; }
.lamp.green.on { background: #22c55e; box-shadow: 0 0 20px #22c55e; }
.lamp.white.on { background: #fff; box-shadow: 0 0 20px #fff; }
.lamp.blue.on { background: #3b82f6; box-shadow: 0 0 20px #3b82f6; }

.text {
  display: flex;
  flex-direction: column;
  gap: 3px;
  min-width: 0;
}

.caption {
  font-size: 0.72rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-dim);
}

.caption.sim {
  color: var(--warn);
}

.value {
  font-size: 0.95rem;
}

.value.red { color: #ef4444; }
.value.green { color: #22c55e; }
.value.amber { color: #f59e0b; }

.exit {
  flex: none;
  height: 56px;
  padding: 0 22px;
  border-left: none;
  border-top: 1px solid var(--line);
  border-radius: 0;
}
</style>
