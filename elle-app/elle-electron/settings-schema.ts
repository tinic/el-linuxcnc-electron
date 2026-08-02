import { z } from 'zod'

/**
 * THE single source of truth for persisted application settings.
 *
 * Adding a setting is a ONE-FILE change: add a field with a `.catch()`
 * default below. The electron-store shape, the IPC payload types, and the
 * frontend refs (via useSettings) all derive from this schema.
 *
 * `.catch(default)` doubles as both the default for missing fields and
 * recovery for corrupt values, so a damaged appConfig.json can never
 * prevent the app from starting.
 */

export const ToolSchema = z.object({
  id: z.number().int(),
  offsetX: z.number().catch(0),
  offsetZ: z.number().catch(0),
  description: z.string().catch('')
})

export type Tool = z.infer<typeof ToolSchema>

const makeDefaultTools = (): Tool[] =>
  Array.from({ length: 10 }, (_, i) => ({
    id: i,
    offsetX: 0,
    offsetZ: 0,
    description: i === 0 ? 'Reference Tool' : ''
  }))

export const SettingsSchema = z.object({
  diameterMode: z.boolean().catch(false),
  defaultMetricOnStartup: z.boolean().catch(true),
  selectedThreadingTab: z.number().int().catch(0),
  selectedTurningTab: z.number().int().catch(0),
  selectedPitchTab: z.array(z.number()).length(2).catch([0, 0]),
  pitchX: z.number().catch(0),
  pitchZ: z.number().catch(0),
  encoderScaleZ: z.number().catch(0.001),
  encoderScaleX: z.number().catch(-0.001),
  tools: z.array(ToolSchema).length(10).catch(() => makeDefaultTools()),
  currentToolIndex: z.number().int().catch(0),
  currentToolOffsetX: z.number().catch(0),
  currentToolOffsetZ: z.number().catch(0)
})

export type Settings = z.infer<typeof SettingsSchema>

/** Fresh, fully-populated defaults (never a shared instance). */
export const settingsDefaults = (): Settings => SettingsSchema.parse({})
