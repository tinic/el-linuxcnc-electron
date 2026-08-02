<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue'

import SettingsScreen from './components/SettingsScreen.vue'
import HalConsole from './components/HalConsole.vue'
import ManualScreen from './components/ManualScreen.vue'
import CannedCyclesScreen from './components/CannedCyclesScreen.vue'
import ToolTable from './components/ToolTable.vue'
import { useHAL } from './composables/useHAL'
import { MenuType, selectedMenu } from './composables/useAppState'
import { useSettings } from './composables/useSettings'
import { useToolTable } from './composables/useToolTable'

const {
  xpos,
  zpos,
  cannedCycleRunning,
  errorState,
  connectionState,
  cleanupCannedCycles,
  startPoll,
  endPoll,
  setAxisOffset,
  xpitch,
  zpitch,
  updateHALOut
} = useHAL()

// Machine status computed from various states
const machineStatus = computed(() => {
  // A lost backend outranks everything: position and error flags are stale
  if (connectionState.value === 'disconnected') {
    return 'offline' // Amber - backend unreachable
  }
  if (connectionState.value === 'connecting') {
    return 'connecting' // Amber - waiting for first contact with HAL
  }

  // Error states take highest priority
  if (errorState.value) {
    return 'error' // Red - machine error/fault
  }

  // Running any cycle takes priority
  if (cannedCycleRunning.value) {
    return 'running' // Green - machine running/in cycle
  }

  // Mode-based status when not running
  if (selectedMenu.value === MenuType.manual) {
    return 'manual' // Blue - manual mode (Home menu)
  } else if (selectedMenu.value === MenuType.cannedCycles) {
    return 'cannedCycle' // White - canned cycle mode ready
  }

  return 'idle' // Fallback - should rarely be used
})

const statusDisplay = computed(() => {
  switch (machineStatus.value) {
    case 'offline':
      return { text: 'OFFLINE', class: 'status-offline', title: 'Backend: connection lost' }
    case 'connecting':
      return { text: 'CONNECTING', class: 'status-connecting', title: 'Backend: waiting for HAL' }
    case 'error':
      return { text: 'ERROR', class: 'status-error', title: 'Machine: Error/Fault State' }
    case 'running':
      return { text: 'RUNNING', class: 'status-running', title: 'Machine: Running/In Cycle' }
    case 'manual':
      return { text: 'MANUAL', class: 'status-manual', title: 'Machine: Manual Mode' }
    case 'cannedCycle':
      return {
        text: 'CANNED CYCLE',
        class: 'status-cannedCycle',
        title: 'Machine: Canned Cycle Mode'
      }
    case 'idle':
      return { text: 'IDLE', class: 'status-idle', title: 'Machine: Idle' }
    default:
      return { text: 'UNKNOWN', class: 'status-unknown', title: 'Machine: Unknown Status' }
  }
})

// Internal
// Get settings from composable
const { pitchX, pitchZ, isQuitting, loadSettings, saveSettings, currentToolIndex, currentToolOffsetX, currentToolOffsetZ } = useSettings()

const { showToolTable } = useToolTable()

const onToolSelected = (toolId: number, offsetX: number, offsetZ: number) => {
  currentToolIndex.value = toolId
  currentToolOffsetX.value = offsetX
  currentToolOffsetZ.value = offsetZ
  showToolTable.value = false
}

const menuItems = ref([
  { separator: true },
  {
    label: 'Manual',
    icon: 'pi pi-fw pi-wrench',
    command: () => {
      selectedMenu.value = MenuType.manual
    }
  },
  {
    label: 'Canned Cycles',
    icon: 'pi pi-fw pi-cog',
    command: () => {
      selectedMenu.value = MenuType.cannedCycles
    }
  },
  {
    label: 'HAL',
    icon: 'pi pi-fw pi-link',
    command: () => {
      selectedMenu.value = MenuType.halStatus
    }
  },
  {
    label: 'Settings',
    icon: 'pi pi-fw pi-sliders-v',
    command: () => {
      selectedMenu.value = MenuType.settings
    }
  },
  { separator: true }
])

const halStdoutText = ref('')

const startHAL = () => {
  halStdoutText.value = ''
  const userAgent = navigator.userAgent.toLowerCase()
  if (userAgent.indexOf(' electron/') > -1) {
    window.api.send('startHAL', {})
    xpos.value = 0
    zpos.value = 0
    setAxisOffset('x', 0)
    setAxisOffset('z', 0)
  }
}

const stopHAL = () => {
  halStdoutText.value = ''
  const userAgent = navigator.userAgent.toLowerCase()
  if (userAgent.indexOf(' electron/') > -1) {
    window.api.send('stopHAL', {})
    xpos.value = 0
    zpos.value = 0
    setAxisOffset('x', 0)
    setAxisOffset('z', 0)
  }
}

const quitApplication = async () => {
  const userAgent = navigator.userAgent.toLowerCase()

  if (userAgent.indexOf(' electron/') > -1) {
    isQuitting.value = true

    // Save settings one final time before quitting using the proper save function
    try {
      await saveSettings()
    } catch (error) {
      console.error('Failed to save final settings:', error)
    }

    window.api.send('quit', {})
  }
}

// Current tool changes are now auto-saved via the settings watcher

onMounted(async () => {
  // Load settings first (includes tool table and current tool)
  await loadSettings()

  // Initialize pitch values from settings
  if (pitchX.value > 0) {
    xpitch.value = pitchX.value
  }
  if (pitchZ.value > 0) {
    zpitch.value = pitchZ.value
  }

  const userAgent = navigator.userAgent.toLowerCase()
  if (userAgent.indexOf(' electron/') > -1) {
    window.api.receive('halStarted', () => {
      selectedMenu.value = MenuType.manual
      startPoll()
      updateHALOut()
    })

    window.api.receive('halStopped', () => {
      endPoll()
      cleanupCannedCycles()
    })

    window.api.receive('halStdout', (event: any, arg: any) => {
      halStdoutText.value += event as string
    })

    selectedMenu.value = MenuType.halStatus
    startHAL()
  } else {
    startPoll()
    updateHALOut()
  }
})

onUnmounted(() => {
  cleanupCannedCycles()
})
</script>

<script lang="ts"></script>

<template>
  <div class="flex flex-row flex-grow-1 absolute top-0 left-0 wrapper">
    <Menu v-model="selectedMenu" :model="menuItems" class="flex-none">
      <template #start>
        <button
          class="w-full p-link flex align-items-center justify-content-start p-2 pl-3 text-color hover:surface-200 border-noround"
        >
          <div class="flex flex-column align">
            <span class="font-bold">Elle</span>
          </div>
        </button>
      </template>
      <template #end>
        <div class="flex flex-column">
          <!-- Industrial Stack Light -->
          <div class="flex align-items-center justify-content-start p-2 pl-4 border-noround">
            <div class="stack-light" :title="statusDisplay.title">
              <div class="stack-light-base"></div>
              <div
                :class="['stack-segment', 'segment-red', { active: machineStatus === 'error' }]"
              ></div>
              <div
                :class="[
                  'stack-segment',
                  'segment-amber',
                  { active: machineStatus === 'offline' || machineStatus === 'connecting' }
                ]"
              ></div>
              <div
                :class="['stack-segment', 'segment-green', { active: machineStatus === 'running' }]"
              ></div>
              <div
                :class="[
                  'stack-segment',
                  'segment-white',
                  { active: machineStatus === 'cannedCycle' }
                ]"
              ></div>
              <div
                :class="['stack-segment', 'segment-blue', { active: machineStatus === 'manual' }]"
              ></div>
            </div>
            <span class="ml-3 text-sm font-semibold">{{ statusDisplay.text }}</span>
          </div>

          <div class="menu-separator"></div>
          <button
            class="w-full p-link flex align-items-center justify-content-start p-2 pl-4 text-color hover:surface-200 border-noround"
            @click="quitApplication"
          >
            <i class="pi pi-sign-out" />
            <span class="ml-2">Exit</span>
          </button>
        </div>
      </template>
    </Menu>
    <ManualScreen v-if="selectedMenu == MenuType.manual" />
    <CannedCyclesScreen v-if="selectedMenu == MenuType.cannedCycles" />
    <HalConsole
      v-if="selectedMenu == MenuType.halStatus"
      :output="halStdoutText"
      @start="startHAL"
      @stop="stopHAL"
    />
    <SettingsScreen v-if="selectedMenu == MenuType.settings" />
  </div>

  <!-- Tool Table Modal -->
  <ToolTable
    :visible="showToolTable"
    :current-tool-id="currentToolIndex"
    @update:visible="showToolTable = $event"
    @tool-selected="onToolSelected"
  />
</template>

<style scoped>

.wrapper,
html,
body {
  width: 100%;
  height: 100%;
  margin: 0;
  color: #ffffff;
  background-color: #222222;
  -webkit-touch-callout: none; /* Safari */
  -webkit-user-select: none; /* Chrome */
  -moz-user-select: none; /* Firefox */
  -ms-user-select: none; /* Internet Explorer/Edge */
  user-select: none;
}

.wrapper {
  display: flex;
  flex-direction: column;
}

/* Simple divider lines for visual separation */

/* Industrial Stack Light Design */
.stack-light {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.stack-light-base {
  width: 36px;
  height: 12px;
  background: #444;
  border-radius: 0 0 6px 6px;
  margin-top: 3px;
}

.stack-segment {
  width: 30px;
  height: 18px;
  margin: 3px 0;
  border-radius: 50%;
  border: 3px solid #333;
  transition: all 0.3s ease;
  position: relative;
}

/* Inactive (darkened) states */
.segment-red {
  background: #441a1a;
  border-color: #662222;
}

.segment-amber {
  background: #442a0a;
  border-color: #664411;
}

.segment-green {
  background: #1a441a;
  border-color: #226622;
}

.segment-blue {
  background: #1a2244;
  border-color: #223366;
}

.segment-white {
  background: #444444;
  border-color: #666666;
}

/* Active (illuminated) states with glow effect */
.segment-red.active {
  background: #ef4444;
  border-color: #dc2626;
  box-shadow: 0 0 24px #ef4444, inset 0 3px 6px rgba(255, 255, 255, 0.3);
}

.segment-amber.active {
  background: #f59e0b;
  border-color: #d97706;
  box-shadow: 0 0 24px #f59e0b, inset 0 3px 6px rgba(255, 255, 255, 0.3);
}

.segment-green.active {
  background: #22c55e;
  border-color: #16a34a;
  box-shadow: 0 0 24px #22c55e, inset 0 3px 6px rgba(255, 255, 255, 0.3);
}

.segment-blue.active {
  background: #3b82f6;
  border-color: #2563eb;
  box-shadow: 0 0 24px #3b82f6, inset 0 3px 6px rgba(255, 255, 255, 0.3);
}

.segment-white.active {
  background: #ffffff;
  border-color: #e5e7eb;
  box-shadow: 0 0 24px #ffffff, inset 0 3px 6px rgba(255, 255, 255, 0.4);
}

/* Threading popover styling */
:deep(.threading-popover .p-popover) {
  background-color: #333 !important;
  border: 1px solid #555 !important;
  color: #ffffff !important;
  font-size: 0.9em;
  max-width: 250px;
}

:deep(.threading-popover .p-popover-arrow) {
  border-bottom-color: #333 !important;
  border-top-color: #333 !important;
  border-left-color: #333 !important;
  border-right-color: #333 !important;
}

.cursor-pointer {
  cursor: pointer;
}

.cursor-pointer:hover {
  color: #aaaaaa;
}

.menu-separator {
  height: 1px;
  background-color: rgba(255, 255, 255, 0.12);
  margin: 0.5rem 0;
}

.bg-primary {
  background-color: #3b82f6 !important;
  color: white !important;
}

/* Debug coordinates styling */
.debug-coordinates {
  font-family: 'iosevka', monospace;
  font-size: 0.75rem;
  line-height: 1.2;
  border-left: 2px solid #555;
  margin-left: 1rem;
}

.coordinate-line {
  margin-bottom: 0.25rem;
  color: #cccccc;
}

.text-color-secondary {
  color: #888888;
  font-weight: bold;
}
</style>
