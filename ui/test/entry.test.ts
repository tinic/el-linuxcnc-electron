import { beforeEach, describe, expect, it } from 'vitest'

import { entryText, press, select, settle, type EntryField, type Key } from '../src/entry'

function field(initial: number | null): EntryField & { committed: number[]; cleared: number } {
  const target = {
    id: 'test',
    value: initial,
    committed: [] as number[],
    cleared: 0,
    current: (): number | null => target.value,
    commit: (value: number): void => {
      target.committed.push(value)
      target.value = value
    },
    clear: (): void => {
      target.cleared += 1
      target.value = null
    },
  }
  return target
}

function type(keys: Key[]): void {
  for (const key of keys) {
    press(key)
  }
}

describe('number entry', () => {
  beforeEach(() => {
    press('escape')
  })

  it('commits typed digits exactly on ENT', () => {
    const target = field(5)
    select(target)
    type(['0', '.', '1', '2', 'enter'])
    expect(target.committed).toEqual([0.12])
  })

  it('shows what is being typed', () => {
    select(field(5))
    type(['1', '.', '5'])
    expect(entryText('test')).toBe('1.5')
    press('sign')
    expect(entryText('test')).toBe('-1.5')
  })

  it('ignores a second decimal point', () => {
    const target = field(0)
    select(target)
    type(['1', '.', '2', '.', '3', 'enter'])
    expect(target.committed).toEqual([1.23])
  })

  it('ESC leaves the value untouched', () => {
    const target = field(5)
    select(target)
    type(['9', 'escape'])
    expect(target.committed).toEqual([])
  })

  it('halves and thirds the selected value immediately', () => {
    const target = field(9)
    select(target)
    press('third')
    expect(target.committed).toEqual([3])
    select(target)
    press('half')
    expect(target.committed).toEqual([3, 1.5])
  })

  it('± on a selected value negates it immediately', () => {
    const target = field(2.5)
    select(target)
    press('sign')
    expect(target.committed).toEqual([-2.5])
  })

  it('tapping another control commits typed digits', () => {
    const target = field(1)
    select(target)
    type(['4', '2'])
    settle()
    expect(target.committed).toEqual([42])
  })

  it('tapping another control without typing changes nothing', () => {
    const target = field(1)
    select(target)
    settle()
    expect(target.committed).toEqual([])
  })

  it('erasing every digit clears the field', () => {
    const target = field(3)
    select(target)
    type(['7', 'backspace'])
    expect(target.cleared).toBe(1)
    expect(target.committed).toEqual([])
  })
})
