import { reactive, ref, toRefs, watch } from 'vue'
import { SettingsSchema, settingsDefaults } from '../../../elle-electron/settings-schema'

// Persisted state — the field list comes from the schema, the single source
// of truth shared with the electron main process. Adding a setting only
// requires a schema change; load, save, and typing all follow.
const persisted = reactive(settingsDefaults())

// Runtime-only state (never persisted)
const metric = ref(true)
const isQuitting = ref(false)

const isElectron = () => navigator.userAgent.toLowerCase().includes(' electron/')

const loadSettings = async () => {
  if (!isElectron()) {
    return
  }
  try {
    if (window.settings && window.settings.get) {
      const stored = await window.settings.get()
      Object.assign(persisted, SettingsSchema.parse(stored))
      metric.value = persisted.defaultMetricOnStartup
    } else {
      console.error('window.settings is not available')
    }
  } catch (error) {
    console.error('Failed to load settings:', error)
  }
}

const saveSettings = async () => {
  if (!isElectron()) {
    return
  }
  try {
    // Parsing through the schema turns the reactive proxy into a plain,
    // structured-clone-safe object — refs and proxies never cross IPC.
    await window.settings.save(SettingsSchema.parse(persisted))
  } catch (error) {
    console.error('Failed to save settings:', error)
  }
}

// Auto-save on any change, including nested tool edits (module-level, so it
// is registered exactly once). Never gate this on isQuitting — the final
// save during quit must always go through.
watch(persisted, () => {
  saveSettings()
}, { deep: true })

export function useSettings() {
  return {
    ...toRefs(persisted),
    metric,
    isQuitting,
    loadSettings,
    saveSettings
  }
}
