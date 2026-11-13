// Press-and-hold jogging. While a jog button is held, the panel re-sends "held" every refreshMs; the
// machine stops the slide by itself when those refreshes stop for 150 ms. The panel side therefore
// must never keep refreshing without a finger (or mouse button) actually on the button:
//
// - The press is tied to the pointer with pointer capture. If the pointer is no longer down when
//   the press arrives (WebKitGTK can deliver a touch's press after its release), capture fails and
//   no jog starts.
// - Every refresh first checks that the button still holds the capture; a lost release (no
//   pointerup, no pointercancel) therefore ends the jog within one refresh.

import type { JogButton } from './protocol'

export const refreshMs = 50

/// The parts of a pressed element the hold needs: pointer capture.
export interface CaptureTarget {
  setPointerCapture(pointerId: number): void
  hasPointerCapture(pointerId: number): boolean
}

export interface JogSink {
  send(button: JogButton, held: boolean): void
}

export interface Timers {
  setInterval(callback: () => void, ms: number): number
  clearInterval(handle: number): void
}

interface Hold {
  timer: number
  target: CaptureTarget
  pointerId: number
}

export class JogHold {
  private readonly holds = new Map<JogButton, Hold>()

  constructor(
    private readonly sink: JogSink,
    private readonly timers: Timers,
  ) {}

  /// A pointer went down on a jog button. Returns whether a jog started.
  press(button: JogButton, target: CaptureTarget, pointerId: number): boolean {
    this.release(button)
    try {
      target.setPointerCapture(pointerId)
    } catch {
      return false // the pointer is not down (any more): there is nothing to hold
    }
    if (!target.hasPointerCapture(pointerId)) {
      return false
    }
    this.sink.send(button, true)
    const timer = this.timers.setInterval(() => {
      if (target.hasPointerCapture(pointerId)) {
        this.sink.send(button, true)
      } else {
        this.release(button)
      }
    }, refreshMs)
    this.holds.set(button, { timer, target, pointerId })
    return true
  }

  /// The pointer left the button: released, cancelled or capture lost.
  release(button: JogButton): void {
    const hold = this.holds.get(button)
    if (hold === undefined) {
      return
    }
    this.timers.clearInterval(hold.timer)
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
