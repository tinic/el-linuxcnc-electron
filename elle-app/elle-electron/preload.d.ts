type AppSettings = import('./settings-schema').Settings

declare interface api {
  send: (channel: string, data: unknown) => void
  receive: (channel: string, func: (...args: unknown[]) => void) => void
}

declare interface settings {
  get: () => Promise<AppSettings>
  save: (settings: AppSettings) => Promise<boolean>
}
