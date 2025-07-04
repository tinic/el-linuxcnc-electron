import Store from 'electron-store'

interface Tool {
  id: number
  offsetX: number
  offsetZ: number
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
    encoderScaleZ?: number
    encoderScaleX?: number
    tools?: Tool[]
    currentToolIndex?: number
    currentToolOffsetX?: number
    currentToolOffsetZ?: number
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
