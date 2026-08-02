import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { ApiError, api, useConnection } from './client'

function okResponse(body: object): Response {
  return new Response(JSON.stringify(body), {
    status: 200,
    headers: { 'Content-Type': 'application/json' }
  })
}

function errorResponse(status: number, body: object): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { 'Content-Type': 'application/json' }
  })
}

const HAL_IN = {
  position_z: 1.5,
  position_x: -0.25,
  position_a: 0.5,
  speed_rps: 10,
  program_running: false,
  error_state: false
}

describe('api client', () => {
  const fetchMock = vi.fn()

  beforeEach(() => {
    vi.stubGlobal('fetch', fetchMock)
    fetchMock.mockReset()
  })

  afterEach(() => {
    vi.unstubAllGlobals()
  })

  it('returns the typed body on success and marks the connection up', async () => {
    fetchMock.mockResolvedValue(okResponse(HAL_IN))

    const halIn = await api.getHalIn()

    expect(halIn.position_z).toBe(1.5)
    expect(halIn.program_running).toBe(false)
    const { connectionState, isConnected } = useConnection()
    expect(connectionState.value).toBe('connected')
    expect(isConnected.value).toBe(true)
  })

  it('surfaces backend validation messages as ApiError with status', async () => {
    fetchMock.mockResolvedValue(
      errorResponse(400, { status: 'Error', message: 'ThreadingParams: Pitch missing' })
    )

    await expect(
      api.generateThreadingGcode({} as never)
    ).rejects.toMatchObject({
      name: 'ApiError',
      status: 400,
      message: 'ThreadingParams: Pitch missing'
    })
  })

  it('throws ApiError on network failure instead of returning {}', async () => {
    fetchMock.mockRejectedValue(new TypeError('fetch failed'))

    await expect(api.abort()).rejects.toBeInstanceOf(ApiError)
  })

  it('declares the connection lost only after consecutive failures', async () => {
    // Start from a healthy connection
    fetchMock.mockResolvedValueOnce(okResponse(HAL_IN))
    await api.getHalIn()
    const { connectionState, lastError } = useConnection()
    expect(connectionState.value).toBe('connected')

    fetchMock.mockRejectedValue(new TypeError('fetch failed'))
    await expect(api.getHalIn()).rejects.toBeInstanceOf(ApiError)
    await expect(api.getHalIn()).rejects.toBeInstanceOf(ApiError)
    // Two failures: still not declared lost (threshold is 3)
    expect(connectionState.value).toBe('connected')

    await expect(api.getHalIn()).rejects.toBeInstanceOf(ApiError)
    expect(connectionState.value).toBe('disconnected')
    expect(lastError.value).toContain('fetch failed')

    // A single success restores the connection
    fetchMock.mockResolvedValue(okResponse(HAL_IN))
    await api.getHalIn()
    expect(connectionState.value).toBe('connected')
    expect(lastError.value).toBeNull()
  })
})
