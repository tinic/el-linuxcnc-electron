// Press-and-hold jogging. While a jog button is held, the panel re-sends "held" every refreshMs; the
// machine stops the slide by itself when those refreshes stop for 150 ms. The panel side therefore
// must never keep refreshing without a finger (or mouse button) actually on the button, so every
// press comes with a check that it is still held, and every refresh asks it first:
//
// - Mouse: the press captures the pointer; it is held while the button keeps the capture. If the
//   pointer is no longer down when the press arrives, capture fails and no jog starts.
// - Touch: WebKitGTK reports a touch at once only as touchstart; pointer and mouse events follow
//   after the finger lifts, as an emulated tap (seen on the lathe over remote desktop:
//   touchstart, touchend, then pointerdown/pointerup/click within 3 ms). Touches are therefore
//   tracked by identifier from touchstart to touchend/touchcancel, and the emulated mouse events
//   that may still follow a touch are ignored.

import type { JogButton } from './protocol'

export const refreshMs = 50

/// How long after a touch mouse events are taken to be WebKit's emulation of that touch.
export const emulatedMouseWindowMs = 800

export interface JogSink {
  send(button: JogButton, held: boolean): void
}

export interface Timers {
  setInterval(callback: () => void, ms: number): number
  clearInterval(handle: number): void
}

/// The parts of a pressed element a mouse press needs: pointer capture.
export interface CaptureTarget {
  setPointerCapture(pointerId: number): void
  hasPointerCapture(pointerId: number): boolean
}

export class JogHold {
  private readonly holds = new Map<JogButton, number>()

  constructor(
    private readonly sink: JogSink,
    private readonly timers: Timers,
  ) {}

  /// Starts jogging while stillHeld() is true. Returns whether a jog started.
  press(button: JogButton, stillHeld: () => boolean): boolean {
    this.release(button)
    if (!stillHeld()) {
      return false
    }
    this.sink.send(button, true)
    const timer = this.timers.setInterval(() => {
      if (stillHeld()) {
        this.sink.send(button, true)
      } else {
        this.release(button)
      }
    }, refreshMs)
    this.holds.set(button, timer)
    return true
  }

  release(button: JogButton): void {
    const timer = this.holds.get(button)
    if (timer === undefined) {
      return
    }
    this.timers.clearInterval(timer)
    this.holds.delete(button)
    this.sink.send(button, false)
  }

  releaseAll(): void {
    for (const button of [...this.holds.keys()]) {
      this.release(button)
    }
  }

  held(button: JogButton): boolean {
    return this.holds.has(button)
  }
}

/// A mouse press: held while the element keeps the pointer capture. Returns null when the pointer
/// cannot be captured because it is not down (any more).
export function capturedPress(target: CaptureTarget, pointerId: number): (() => boolean) | null {
  try {
    target.setPointerCapture(pointerId)
  } catch {
    return null
  }
  return () => target.hasPointerCapture(pointerId)
}

/// The touches currently down, by identifier, and when the last touch began or ended.
export class Touches {
  private readonly down = new Set<number>()
  private lastActivity = Number.NEGATIVE_INFINITY

  start(identifier: number, now: number): () => boolean {
    this.down.add(identifier)
    this.lastActivity = now
    return () => this.down.has(identifier)
  }

  /// A touch that is not a jog press (STOP, say): only its emulated mouse events must be ignored.
  touched(now: number): void {
    this.lastActivity = now
  }

  end(identifier: number, now: number): void {
    this.down.delete(identifier)
    this.lastActivity = now
  }

  /// Mouse events this soon after a touch are WebKit's emulation of it, not a mouse.
  emulatingMouse(now: number): boolean {
    return now - this.lastActivity < emulatedMouseWindowMs
  }
}
