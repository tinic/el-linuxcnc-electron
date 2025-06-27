import Store from 'electron-store'

interface Tool {
  id: number
  offset: number
  description: string
}

interface AppConfig {
  setting: {
    appBounds?: any
    diameterMode?: boolean
    defaultMetricOnStartup?: boolean
    selectedThreadingTab?: number
    selectedTurningTab?: number
    selectedPitchTab?: number[]
    pitchX?: number
    pitchZ?: number
    tools?: Tool[]
    currentToolIndex?: number
    currentToolOffset?: number
  }
}

export const appConfig = new Store<AppConfig>({
  name: 'appConfig',
  defaults: {
    setting: {
      diameterMode: false,
      defaultMetricOnStartup: true
    }
  },
  schema: {
    setting: {
      type: 'object'
    }
  }
})
