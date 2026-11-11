// On-screen number entry, shared by every numeric field.
//
// Behaviour of the original panel, kept: tapping a field selects it and shows its value; digits
// replace it; ENT commits, ESC cancels; ± and the ⅓ / ½ keys act on the selected value directly;
// tapping another control commits what was typed. Fixed here: typed numbers are parsed exactly
// (no digit-by-digit float arithmetic), a second decimal point is ignored, and tapping elsewhere
// without typing leaves the value alone instead of re-committing a possibly stale reading.

import { computed, reactive } from 'vue'

export interface EntryField {
  /** Unique per field, e.g. "dro.x" or "threading.pitch". */
  id: string
  /** Current value in display units, or null when the field is empty. */
  current: () => number | null
  /** Stores a value given in display units. */
  commit: (value: number) => void
  /** Empties the field (erasing every digit). Fields that cannot be empty omit this. */
  clear?: () => void
}

type Stage = 'idle' | 'selected' | 'typing'

const maxDigits = 12

interface EntryState {
  stage: Stage
  fieldId: string | null
  typed: string
  negative: boolean
  previous: number
}

const state = reactive<EntryState>({
  stage: 'idle',
  fieldId: null,
  typed: '',
  negative: false,
  previous: 0,
})

let field: EntryField | null = null

function typedValue(): number {
  const parsed = state.typed === '' || state.typed === '.' ? 0 : Number.parseFloat(state.typed)
  return state.negative ? -parsed : parsed
}

function reset(): void {
  state.stage = 'idle'
  state.fieldId = null
  state.typed = ''
  state.negative = false
  field = null
}

function finish(value: number): void {
  const target = field
  reset()
  target?.commit(value)
}

/** Commits pending typing; called before any other control acts. */
export function settle(): void {
  if (state.stage === 'typing') {
    finish(typedValue())
  } else {
    reset()
  }
}

export function select(target: EntryField): void {
  settle()
  field = target
  state.fieldId = target.id
  state.stage = 'selected'
  state.previous = target.current() ?? 0
  state.typed = ''
  state.negative = false
}

export type Key =
  | '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' | '.'
  | 'enter' | 'escape' | 'backspace' | 'sign' | 'third' | 'half'

export function press(key: Key): void {
  if (state.stage === 'idle' || field === null) {
    return
  }
  switch (key) {
    case 'enter':
      if (state.stage === 'typing') {
        finish(typedValue())
      } else {
        reset()
      }
      return
    case 'escape':
      reset()
      return
    case 'backspace':
      state.stage = 'typing'
      state.typed = state.typed.slice(0, -1)
      if (state.typed === '') {
        const target = field
        reset()
        target.clear?.()
      }
      return
    case 'sign':
      if (state.stage === 'selected') {
        finish(-state.previous)
      } else {
        state.negative = !state.negative
      }
      return
    case 'third':
    case 'half':
      if (state.stage === 'selected') {
        finish(state.previous / (key === 'third' ? 3 : 2))
      }
      return
    case '.':
      if (state.typed.includes('.')) {
        return
      }
      break
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      break
  }
  if (state.typed.length < maxDigits) {
    state.stage = 'typing'
    state.typed += key
  }
}

/** What a field should display while selected: the typed text, or null when it shows its value. */
export function entryText(id: string): string | null {
  if (state.fieldId !== id || state.stage !== 'typing') {
    return null
  }
  return `${state.negative ? '-' : ''}${state.typed === '' ? '0' : state.typed}`
}

export const activeField = computed(() => state.fieldId)
