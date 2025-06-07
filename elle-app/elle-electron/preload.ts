import { contextBridge, ipcRenderer } from "electron";

contextBridge.exposeInMainWorld("browserWindow", {
    versions: () => ipcRenderer.invoke("versions"),
});

contextBridge.exposeInMainWorld("settings", {
    get: () => ipcRenderer.invoke("getSettings"),
    save: (settings: any) => ipcRenderer.invoke("saveSettings", settings),
});

contextBridge.exposeInMainWorld('api', {
    send: (channel: any, data: any) => {
      let validChannels = ['startHAL','stopHAL','quit']
      if (validChannels.includes(channel)) {
        ipcRenderer.send(channel, data)
      }
    },
    receive: (channel: any, func: any) => {
      let validChannels = ['halStarted','halStopped','halStdout']
      if (validChannels.includes(channel)) {
        ipcRenderer.on(channel, (event, ...args) => func(...args))
      }
    }
})
