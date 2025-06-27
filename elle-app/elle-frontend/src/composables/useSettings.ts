import { ref, watch } from 'vue'

// Import tool types
interface Tool {
  id: number
  offset: number
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
  { id: 0, offset: 0, description: 'Reference Tool' },
  { id: 1, offset: 0, description: '' },
  { id: 2, offset: 0, description: '' },
  { id: 3, offset: 0, description: '' },
  { id: 4, offset: 0, description: '' },
  { id: 5, offset: 0, description: '' },
  { id: 6, offset: 0, description: '' },
  { id: 7, offset: 0, description: '' },
  { id: 8, offset: 0, description: '' },
  { id: 9, offset: 0, description: '' }
])

const currentToolIndex = ref(0)
const currentToolOffset = ref(0)

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
          if (settings.currentToolOffset !== undefined) {
            currentToolOffset.value = settings.currentToolOffset
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
    if (isQuitting.value) {return} // Don't save settings when quitting

    const userAgent = navigator.userAgent.toLowerCase()
    if (userAgent.indexOf(' electron/') > -1) {
      try {
        // Convert reactive arrays to plain objects for serialization
        const plainTools = JSON.parse(JSON.stringify(tools.value))
        
        await window.settings.save({
          diameterMode: diameterMode.value,
          defaultMetricOnStartup: defaultMetricOnStartup.value,
          selectedThreadingTab: selectedThreadingTab.value,
          selectedTurningTab: selectedTurningTab.value,
          selectedPitchTab: selectedPitchTab.value,
          pitchX: pitchX.value,
          pitchZ: pitchZ.value,
          tools: plainTools,
          currentToolIndex: currentToolIndex.value,
          currentToolOffset: currentToolOffset.value
        })
      } catch (error) {
        console.error('Failed to save settings:', error)
      }
    }
  }

  // Auto-save settings when they change (only set up once)
  if (!isWatcherSetup) {
    watch([diameterMode, defaultMetricOnStartup, selectedThreadingTab, selectedTurningTab, selectedPitchTab, pitchX, pitchZ, tools, currentToolIndex, currentToolOffset], () => {
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
    currentToolOffset
  }
}
