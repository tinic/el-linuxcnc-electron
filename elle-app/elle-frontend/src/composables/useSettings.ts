import { ref, watch } from 'vue'

export function useSettings() {
  const metric = ref(true)
  const diameterMode = ref(false)
  const defaultMetricOnStartup = ref(true)
  const selectedThreadingTab = ref(0)
  const selectedTurningTab = ref(0)
  const selectedPitchTab = ref([0, 0]) // [x-axis, z-axis]
  const pitchX = ref(0.0)
  const pitchZ = ref(0.0)
  const isQuitting = ref(false)

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
        await window.settings.save({
          diameterMode: diameterMode.value,
          defaultMetricOnStartup: defaultMetricOnStartup.value,
          selectedThreadingTab: selectedThreadingTab.value,
          selectedTurningTab: selectedTurningTab.value,
          selectedPitchTab: selectedPitchTab.value,
          pitchX: pitchX.value,
          pitchZ: pitchZ.value
        })
      } catch (error) {
        console.error('Failed to save settings:', error)
      }
    }
  }

  // Auto-save settings when they change
  watch([diameterMode, defaultMetricOnStartup, selectedThreadingTab, selectedTurningTab, selectedPitchTab, pitchX, pitchZ], () => {
    saveSettings()
  })

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
    saveSettings
  }
}
