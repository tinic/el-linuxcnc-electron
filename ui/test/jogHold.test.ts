import { describe, expect, it } from 'vitest'

import { JogHold, refreshMs, type CaptureTarget, type Timers } from '../src/jogHold'
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

/// A button the pointer is on (down) or not; capture works only while the pointer is down.
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

describe('jog hold', () => {
  it('jogs while the button is held and stops on release', () => {
    const { hold, timers, sent } = setup()
    const button = new FakeButton()
    expect(hold.press('z_minus', button, 1)).toBe(true)
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

  it('does not jog when the press arrives after the touch was already released', () => {
    // WebKitGTK on a touch screen can deliver pointerup before pointerdown.
    const { hold, timers, sent } = setup()
    const button = new FakeButton()
    hold.release('z_minus')
    button.lift()
    expect(hold.press('z_minus', button, 1)).toBe(false)
    timers.tick()
    expect(sent).toEqual([])
    expect(timers.active.size).toBe(0)
  })

  it('stops by itself when the release event is lost', () => {
    const { hold, timers, sent } = setup()
    const button = new FakeButton()
    hold.press('x_plus', button, 7)
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

  it('stop releases every held button', () => {
    const { hold, sent } = setup()
    hold.press('z_plus', new FakeButton(), 1)
    hold.press('x_minus', new FakeButton(), 2)
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
