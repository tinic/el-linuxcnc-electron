// Transport between the panel and the machine.
//
// Inside the elle executable the page talks to C++ in-process: commands go out through WebKit's
// script message handler `elle`, and the executable calls `window.elleReceive(json)` with every
// message for the page. There is no network socket. In a plain browser (UI development) a built-in
// simulator answers instead.

import type { FromMachine, ToMachine } from './protocol'
import { createSimulator } from './simulator'

export type Listener = (message: FromMachine) => void

export interface Transport {
  readonly simulated: boolean
  send: (message: ToMachine) => void
  listen: (listener: Listener) => void
}

interface ScriptMessageHandler {
  postMessage: (body: string) => void
}

declare global {
  interface Window {
    webkit?: { messageHandlers?: { elle?: ScriptMessageHandler } }
    elleReceive?: (json: string) => void
  }
}

function webkitTransport(handler: ScriptMessageHandler): Transport {
  const listeners: Listener[] = []
  window.elleReceive = (json: string): void => {
    const message = JSON.parse(json) as FromMachine
    for (const listener of listeners) {
      listener(message)
    }
  }
  return {
    simulated: false,
    send: (message): void => {
      handler.postMessage(JSON.stringify(message))
    },
    listen: (listener): void => {
      listeners.push(listener)
    },
  }
}

export function createTransport(): Transport {
  const handler = window.webkit?.messageHandlers?.elle
  return handler ? webkitTransport(handler) : createSimulator()
}
