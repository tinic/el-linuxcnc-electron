import Store from 'electron-store'

interface AppConfig {
  setting: {
    appBounds?: any
    diameterMode?: boolean
    defaultMetricOnStartup?: boolean
  }
}

export const appConfig = new Store<AppConfig>({
  name: 'appConfig',
  defaults: {
    setting: {
      diameterMode: false,
      defaultMetricOnStartup: true,
    },
  },
  schema: {
    setting: {
      type: 'object',
    },
  },
})
