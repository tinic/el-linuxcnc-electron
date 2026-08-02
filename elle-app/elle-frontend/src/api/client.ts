/**
 * Typed HTTP client for the Elle HAL backend.
 *
 * All request/response types come from types.gen.ts, which is generated
 * from the pydantic models in elle-hal/api_models.py (`yarn generate:api`).
 *
 * Every call goes through request(), which maintains a shared connection
 * state machine — consumers can watch useConnection() instead of guessing
 * from silently-empty responses. Functions here THROW ApiError on any
 * failure; use the never-throwing wrappers in useHAL for legacy call sites.
 */

import { computed, readonly, ref } from 'vue'
import type {
  CleanupResponse,
  GcodeResponse,
  HalIn,
  HalOut,
  StatusResponse,
  ThreadingExecuteParams,
  ThreadingParams,
  TurningExecuteParams,
  TurningParams
} from './types.gen'

const isElectron = navigator.userAgent.toLowerCase().includes(' electron/')
// In Electron the backend runs on the same machine; in a plain browser we
// assume remote development against the lathe controller.
const HAL_HOST = isElectron ? 'localhost' : 'lathev2'
const HAL_BASE = `http://${HAL_HOST}:8000`
const LINUXCNC_BASE = `http://${HAL_HOST}:8001`

export class ApiError extends Error {
  readonly status: number | null

  constructor(message: string, status: number | null = null) {
    super(message)
    this.name = 'ApiError'
    this.status = status
  }
}

export type ConnectionState = 'connecting' | 'connected' | 'disconnected'

// Shared across the whole app: one backend, one connection state.
const connectionState = ref<ConnectionState>('connecting')
const lastError = ref<string | null>(null)
let consecutiveFailures = 0
// Roughly 1/10th second of missed polls at 30 Hz before declaring loss.
const FAILURE_THRESHOLD = 3

function noteSuccess() {
  consecutiveFailures = 0
  if (connectionState.value !== 'connected') {
    connectionState.value = 'connected'
  }
  lastError.value = null
}

function noteFailure(message: string) {
  consecutiveFailures += 1
  lastError.value = message
  if (consecutiveFailures >= FAILURE_THRESHOLD) {
    connectionState.value = 'disconnected'
  }
}

export function useConnection() {
  return {
    connectionState: readonly(connectionState),
    lastError: readonly(lastError),
    isConnected: computed(() => connectionState.value === 'connected')
  }
}

async function request<T>(
  base: string,
  path: string,
  init: RequestInit = {},
  timeoutMs = 5000
): Promise<T> {
  const controller = new AbortController()
  const timer = setTimeout(() => controller.abort(), timeoutMs)
  try {
    const response = await fetch(base + path, {
      headers: { 'Content-Type': 'application/json' },
      signal: controller.signal,
      ...init
    })
    const body = await response.json().catch(() => null)
    if (!response.ok) {
      const message =
        (body && typeof body.message === 'string' && body.message) ||
        `${path} failed with HTTP ${response.status}`
      throw new ApiError(message, response.status)
    }
    noteSuccess()
    return body as T
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error)
    noteFailure(message)
    if (error instanceof ApiError) {
      throw error
    }
    throw new ApiError(message)
  } finally {
    clearTimeout(timer)
  }
}

function put<T>(path: string, data: object, timeoutMs?: number): Promise<T> {
  return request<T>(HAL_BASE, path, {
    method: 'PUT',
    body: JSON.stringify(data)
  }, timeoutMs)
}

export const api = {
  /** Position/status snapshot; short timeout so loss is detected quickly. */
  getHalIn(): Promise<HalIn> {
    return request<HalIn>(HAL_BASE, '/hal/hal_in', {}, 1000)
  },

  putHalOut(halOut: HalOut): Promise<StatusResponse> {
    return put('/hal/hal_out', halOut)
  },

  generateThreadingGcode(params: ThreadingParams): Promise<GcodeResponse> {
    return put('/hal/threading/generate', params)
  },

  executeThreading(params: ThreadingExecuteParams): Promise<GcodeResponse> {
    return put('/hal/threading', params)
  },

  generateTurningGcode(params: TurningParams): Promise<GcodeResponse> {
    return put('/hal/turning/generate', params)
  },

  executeTurning(params: TurningExecuteParams): Promise<GcodeResponse> {
    return put('/hal/turning', params)
  },

  abort(): Promise<StatusResponse> {
    return put('/hal/abort', {})
  },

  emergencyStop(): Promise<StatusResponse> {
    return put('/hal/estop', {})
  },

  cleanupCannedCycles(): Promise<CleanupResponse> {
    return put('/hal/cleanup', {})
  },

  /** Auxiliary LinuxCNC service (port 8001); not yet part of the schema. */
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  putLinuxCNC(command: string, data: object): Promise<any> {
    return request(LINUXCNC_BASE, `/linuxcnc/${command}`, {
      method: 'PUT',
      body: JSON.stringify(data)
    })
  }
}
