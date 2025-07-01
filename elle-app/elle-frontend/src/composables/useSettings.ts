import { ref, watch } from 'vue'

// Import tool types
interface Tool {
  id: number
  offsetX: number
  offsetZ: number
  description: string
}

// Global state - created once and shared across all components
const metric = ref(true)
const diameterMode = ref(false)
const defaultMetricOnStartup = ref(true)
const selectedThreadingTab = ref(0)
const selectedTurningTab = ref(0)
const selectedPitchTab = ref([0, 0]) // [x-axis, z-axis]
const pitchX = ref(0.0)
const pitchZ = ref(0.0)
const isQuitting = ref(false)

// Tool table state
const tools = ref<Tool[]>([
  { id: 0, offsetX: 0, offsetZ: 0, description: 'Reference Tool' },
  { id: 1, offsetX: 0, offsetZ: 0, description: '' },
  { id: 2, offsetX: 0, offsetZ: 0, description: '' },
  { id: 3, offsetX: 0, offsetZ: 0, description: '' },
  { id: 4, offsetX: 0, offsetZ: 0, description: '' },
  { id: 5, offsetX: 0, offsetZ: 0, description: '' },
  { id: 6, offsetX: 0, offsetZ: 0, description: '' },
  { id: 7, offsetX: 0, offsetZ: 0, description: '' },
  { id: 8, offsetX: 0, offsetZ: 0, description: '' },
  { id: 9, offsetX: 0, offsetZ: 0, description: '' }
])

const currentToolIndex = ref(0)
const currentToolOffsetX = ref(0)
const currentToolOffsetZ = ref(0)

// Flag to ensure watcher is only set up once
let isWatcherSetup = false

export function useSettings() {

  const loadSettings = async () => {
    const userAgent = navigator.userAgent.toLowerCase()
    if (userAgent.indexOf(' electron/') > -1) {
      try {
        if (window.settings && window.settings.get) {
          const settings = await window.settings.get()
          
          diameterMode.value = settings.diameterMode
          defaultMetricOnStartup.value = settings.defaultMetricOnStartup
          selectedThreadingTab.value = settings.selectedThreadingTab || 0
          selectedTurningTab.value = settings.selectedTurningTab || 0
          selectedPitchTab.value = settings.selectedPitchTab || [0, 0]
          pitchX.value = settings.pitchX || 0.0
          pitchZ.value = settings.pitchZ || 0.0
          metric.value = settings.defaultMetricOnStartup
          
          // Load tool table
          if (settings.tools && Array.isArray(settings.tools)) {
            tools.value = settings.tools
          }
          
          // Load current tool
          if (settings.currentToolIndex !== undefined) {
            currentToolIndex.value = settings.currentToolIndex
          }
          if (settings.currentToolOffsetX !== undefined) {
            currentToolOffsetX.value = settings.currentToolOffsetX
          }
          if (settings.currentToolOffsetZ !== undefined) {
            currentToolOffsetZ.value = settings.currentToolOffsetZ
          }
        } else {
          console.error('window.settings is not available')
        }
      } catch (error) {
        console.error('Failed to load settings:', error)
      }
    }
  }

  const saveSettings = async () => {
    const userAgent = navigator.userAgent.toLowerCase()
    if (userAgent.indexOf(' electron/') > -1) {
      try {
        // Convert reactive refs to plain values for serialization
        const settingsToSave = {
          diameterMode: diameterMode.value,
          defaultMetricOnStartup: defaultMetricOnStartup.value,
          selectedThreadingTab: selectedThreadingTab.value,
          selectedTurningTab: selectedTurningTab.value,
          selectedPitchTab: [...selectedPitchTab.value],
          pitchX: pitchX.value,
          pitchZ: pitchZ.value,
          tools: tools.value.map(tool => ({
            id: tool.id,
            offsetX: tool.offsetX,
            offsetZ: tool.offsetZ,
            description: tool.description
          })),
          currentToolIndex: currentToolIndex.value,
          currentToolOffsetX: currentToolOffsetX.value,
          currentToolOffsetZ: currentToolOffsetZ.value
        }

        await window.settings.save(settingsToSave)
      } catch (error) {
        console.error('Failed to save settings:', error)
      }
    }
  }

  // Auto-save settings when they change (only set up once)
  if (!isWatcherSetup) {
    watch([diameterMode, defaultMetricOnStartup, selectedThreadingTab, selectedTurningTab, selectedPitchTab, pitchX, pitchZ, tools, currentToolIndex, currentToolOffsetX, currentToolOffsetZ], () => {
      saveSettings()
    }, { deep: true })
    isWatcherSetup = true
  }

  return {
    metric,
    diameterMode,
    defaultMetricOnStartup,
    selectedThreadingTab,
    selectedTurningTab,
    selectedPitchTab,
    pitchX,
    pitchZ,
    isQuitting,
    loadSettings,
    saveSettings,
    tools,
    currentToolIndex,
    currentToolOffsetX,
    currentToolOffsetZ
  }
}
