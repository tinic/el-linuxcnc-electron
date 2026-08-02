import Store from 'electron-store'
import type { Settings } from '../settings-schema.js'

interface AppConfig {
  setting: Partial<Settings> & {
    appBounds?: { x: number; y: number; width: number; height: number }
  }
}

export const appConfig = new Store<AppConfig>({
  name: 'appConfig',
  defaults: {
    setting: {}
  },
  schema: {
    setting: {
      type: 'object'
    }
  }
})
