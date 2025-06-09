import { ref, watch } from 'vue';

export function useSettings() {
  const metric = ref(true);
  const diameterMode = ref(false);
  const defaultMetricOnStartup = ref(true);
  const isQuitting = ref(false);

  const loadSettings = async () => {
    const userAgent = navigator.userAgent.toLowerCase();
    if (userAgent.indexOf(" electron/") > -1) {
      try {
        if (window.settings && window.settings.get) {
          const settings = await window.settings.get();
          diameterMode.value = settings.diameterMode;
          defaultMetricOnStartup.value = settings.defaultMetricOnStartup;
          metric.value = settings.defaultMetricOnStartup;
        } else {
          console.error("window.settings is not available");
        }
      } catch (error) {
        console.error("Failed to load settings:", error);
      }
    }
  };

  const saveSettings = async () => {
    if (isQuitting.value) return; // Don't save settings when quitting
    
    const userAgent = navigator.userAgent.toLowerCase();
    if (userAgent.indexOf(" electron/") > -1) {
      try {
        await window.settings.save({
          diameterMode: diameterMode.value,
          defaultMetricOnStartup: defaultMetricOnStartup.value,
        });
      } catch (error) {
        console.error("Failed to save settings:", error);
      }
    }
  };

  // Auto-save settings when they change
  watch([diameterMode, defaultMetricOnStartup], () => {
    saveSettings();
  });

  return {
    metric,
    diameterMode,
    defaultMetricOnStartup,
    isQuitting,
    loadSettings,
    saveSettings,
  };
}