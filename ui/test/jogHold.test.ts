import { describe, expect, it } from 'vitest'

import {
  JogHold,
  Touches,
  capturedPress,
  emulatedMouseWindowMs,
  refreshMs,
  type CaptureTarget,
  type Timers,
} from '../src/jogHold'
import type { JogButton } from '../src/protocol'

class FakeTimers implements Timers {
  private next = 1
  readonly active = new Map<number, () => void>()

  setInterval(callback: () => void): number {
    const handle = this.next++
    this.active.set(handle, callback)
    return handle
  }

  clearInterval(handle: number): void {
    this.active.delete(handle)
  }

  tick(): void {
    for (const callback of [...this.active.values()]) {
      callback()
    }
  }
}

/// A button the mouse pointer is on (down) or not; capture works only while the pointer is down.
class FakeButton implements CaptureTarget {
  down = true
  captured = false

  setPointerCapture(): void {
    if (!this.down) {
      throw new DOMException('pointer is not active', 'NotFoundError')
    }
    this.captured = true
  }

  hasPointerCapture(): boolean {
    return this.captured && this.down
  }

  lift(): void {
    this.down = false
    this.captured = false
  }
}

function setup(): { hold: JogHold; timers: FakeTimers; sent: [JogButton, boolean][] } {
  const sent: [JogButton, boolean][] = []
  const timers = new FakeTimers()
  const hold = new JogHold(
    {
      send: (button: JogButton, held: boolean): void => {
        sent.push([button, held])
      },
    },
    timers,
  )
  return { hold, timers, sent }
}

describe('jog hold with a mouse', () => {
  it('jogs while the button is held and stops on release', () => {
    const { hold, timers, sent } = setup()
    const stillHeld = capturedPress(new FakeButton(), 1)
    expect(stillHeld).not.toBeNull()
    expect(hold.press('z_minus', stillHeld ?? ((): boolean => false))).toBe(true)
    timers.tick()
    timers.tick()
    hold.release('z_minus')
    expect(sent).toEqual([
      ['z_minus', true],
      ['z_minus', true],
      ['z_minus', true],
      ['z_minus', false],
    ])
    expect(timers.active.size).toBe(0)
  })

  it('does not jog when the press arrives after the pointer was already released', () => {
    const button = new FakeButton()
    button.lift()
    expect(capturedPress(button, 1)).toBeNull()
  })

  it('stops by itself when the release event is lost', () => {
    const { hold, timers, sent } = setup()
    const button = new FakeButton()
    hold.press('x_plus', capturedPress(button, 7) ?? ((): boolean => false))
    button.lift() // no pointerup, no pointercancel
    timers.tick()
    expect(sent).toEqual([
      ['x_plus', true],
      ['x_plus', false],
    ])
    expect(hold.held('x_plus')).toBe(false)
    timers.tick()
    expect(sent).toHaveLength(2)
  })
})

describe('jog hold with touch', () => {
  it('jogs from touchstart to touchend', () => {
    const { hold, timers, sent } = setup()
    const touches = new Touches()
    hold.press('z_minus', touches.start(3, 1000))
    timers.tick()
    touches.end(3, 1500)
    hold.release('z_minus')
    expect(sent).toEqual([
      ['z_minus', true],
      ['z_minus', true],
      ['z_minus', false],
    ])
  })

  it('stops by itself when the touch ended elsewhere', () => {
    const { hold, timers, sent } = setup()
    const touches = new Touches()
    hold.press('z_plus', touches.start(5, 0))
    touches.end(5, 100) // seen by the window-level listener, not by the button
    timers.tick()
    expect(sent).toEqual([
      ['z_plus', true],
      ['z_plus', false],
    ])
  })

  it('treats the mouse events WebKit emulates after a touch as part of that touch', () => {
    // Recorded on the lathe: touchstart 52685, touchend 53156, then pointerdown/up at 53157-53160.
    const touches = new Touches()
    touches.start(1, 52685)
    touches.end(1, 53156)
    expect(touches.emulatingMouse(53157)).toBe(true)
    expect(touches.emulatingMouse(53160)).toBe(true)
    expect(touches.emulatingMouse(53156 + emulatedMouseWindowMs)).toBe(false)
  })

  it('takes mouse events as a real mouse when no touch happened', () => {
    expect(new Touches().emulatingMouse(0)).toBe(false)
  })

  it('a touch on STOP also marks its emulated mouse events', () => {
    const touches = new Touches()
    touches.touched(2000)
    expect(touches.emulatingMouse(2005)).toBe(true)
  })
})

describe('jog hold', () => {
  it('stop releases every held button', () => {
    const { hold, sent } = setup()
    const touches = new Touches()
    hold.press('z_plus', touches.start(1, 0))
    hold.press('x_minus', touches.start(2, 0))
    hold.releaseAll()
    expect(sent.filter(([, held]) => !held)).toEqual([
      ['z_plus', false],
      ['x_minus', false],
    ])
  })

  it('refreshes faster than the machine-side 150 ms timeout', () => {
    expect(refreshMs).toBeLessThan(150 / 2)
  })
})
