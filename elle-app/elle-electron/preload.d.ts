declare interface api {
  send: (channel: any, data: any) => void
  receive: (channel: any, func: any) => void
}

declare interface settings {
  get: () => Promise<{ diameterMode: boolean; defaultMetricOnStartup: boolean; selectedThreadingTab: number; selectedTurningTab: number; selectedPitchTab: number[]; pitchX: number; pitchZ: number }>
  save: (settings: { diameterMode: boolean; defaultMetricOnStartup: boolean; selectedThreadingTab: number; selectedTurningTab: number; selectedPitchTab: number[]; pitchX: number; pitchZ: number }) => Promise<boolean>
}
