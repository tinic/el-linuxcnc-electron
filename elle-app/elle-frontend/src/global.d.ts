/// <reference path="../../elle-electron/preload.d.ts" />

export {}

declare global {
  interface Window {
    api: api
    settings: settings
  }
}
