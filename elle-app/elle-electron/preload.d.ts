declare interface api {
  send: (channel: any, data: any) => void
  receive: (channel: any, func: any) => void
}

declare interface Tool {
  id: number
  offsetX: number
  offsetZ: number
  description: string
}

declare interface settings {
  get: () => Promise<{ diameterMode: boolean; defaultMetricOnStartup: boolean; selectedThreadingTab: number; selectedTurningTab: number; selectedPitchTab: number[]; pitchX: number; pitchZ: number; encoderScaleZ: number; encoderScaleX: number; tools?: Tool[]; currentToolIndex?: number; currentToolOffsetX?: number; currentToolOffsetZ?: number }>
  save: (settings: { diameterMode: boolean; defaultMetricOnStartup: boolean; selectedThreadingTab: number; selectedTurningTab: number; selectedPitchTab: number[]; pitchX: number; pitchZ: number; encoderScaleZ: number; encoderScaleX: number; tools?: Tool[]; currentToolIndex?: number; currentToolOffsetX?: number; currentToolOffsetZ?: number }) => Promise<boolean>
}
