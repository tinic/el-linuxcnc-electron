// Messages between the operator panel and the elle executable.
//
// Every length is in millimetres and every X value is a radius: unit and diameter conversion
// happen only for display, in units.ts. Field names match the C++ structs one to one.

export type Mode = 'manual' | 'cycle' | 'other'
export type Feed = 'longitudinal' | 'cross' | 'front_compound' | 'back_compound'
export type Direction = 'forward' | 'reverse' | 'hold' | 'idle'
export type JogButton = 'x_minus' | 'x_plus' | 'z_minus' | 'z_plus'
export type Axis = 'x' | 'z' | 'a'

export interface Pitches {
  x: number
  z: number
}

export interface Tool {
  id: number
  offset_x: number
  offset_z: number
  description: string
}

export interface MachineSettings {
  encoder_scale_z: number
  encoder_scale_x: number
  pitches: Pitches
  tools: Tool[]
  current_tool: number
}

export interface Settings {
  diameter_mode: boolean
  metric_on_startup: boolean
  thread_preset_tab: number
  turning_preset_tab: number
  pitch_preset_tabs: [number, number]
  /** The tool is behind the work: drawings show X+ up. Front (default): X+ toward the operator, down. */
  back_tool_post: boolean
  machine: MachineSettings
}

export interface Readout {
  x: number
  z: number
  a_degrees: number
  rpm: number
}

export interface Leadscrew {
  cycle_control: boolean
  z_feed: number
  x_feed: number
  z_driver_enabled: boolean
  x_driver_enabled: boolean
  z_follows_spindle: boolean
  x_follows_spindle: boolean
}

export interface TaskState {
  connected: boolean
  program_running: boolean
  error: boolean
}

export interface MachineStatus {
  readout: Readout
  mode: Mode
  feed: Feed
  direction: Direction
  pitches: Pitches
  leadscrew: Leadscrew
  task: TaskState
}

/**
 * The operator's threading entries (conversational style: positive numbers, the side chosen once,
 * an infeed angle, the length from the face). Null fields select the cycle's default. The backend
 * turns this into the planner's request.
 */
export interface ThreadingForm {
  pitch: number | null
  internal: boolean
  depth: number | null
  allowance: number | null
  infeed_deg: number | null
  length: number | null
  taper_deg: number | null
  run_out: number | null
  retract: number | null
  first_cut: number | null
  cut_factor: number | null
  min_cut: number | null
  spring_cuts: number | null
}

export interface TurningForm {
  target: number | null
  length: number | null
  feed_rate: number | null
  step_down: number | null
  final_step_down: number | null
  taper_angle_deg: number | null
  spring_passes: number | null
  chamfer: number | null
}

export type CycleKind = 'threading' | 'turning'

export interface PlotPoint {
  x: number
  z: number
}

export interface Segment {
  kind: 'rapid' | 'feed' | 'thread'
  from: PlotPoint
  to: PlotPoint
  line: number
}

export interface Backplot {
  segments: Segment[]
  bounds: { min: PlotPoint; max: PlotPoint }
}

export interface ParameterIssue {
  field: string
  problem: string
}

/** The answer to a plan request: either issues to fix, or a program ready to preview and run. */
export interface CyclePlan {
  kind: CycleKind
  issues: ParameterIssue[]
  error: string
  gcode: string[]
  backplot: Backplot | null
  start: PlotPoint
}

export type FromMachine =
  | { type: 'status'; status: MachineStatus }
  | { type: 'settings'; settings: Settings }
  | { type: 'log'; lines: string[] }
  | { type: 'plan'; plan: CyclePlan }
  | { type: 'notice'; level: 'info' | 'error'; text: string }

export interface Preferences {
  diameter_mode: boolean
  metric_on_startup: boolean
  thread_preset_tab: number
  turning_preset_tab: number
  pitch_preset_tabs: [number, number]
  encoder_scale_z: number
  encoder_scale_x: number
  back_tool_post: boolean
}

export type ToMachine =
  | { type: 'ready' }
  | { type: 'select_mode'; mode: Mode }
  | { type: 'set_feed'; feed: Feed }
  | { type: 'set_direction'; direction: Direction }
  | { type: 'set_pitches'; pitches: Pitches }
  | { type: 'set_axis'; axis: Axis; value: number }
  | { type: 'jog'; button: JogButton; held: boolean }
  | { type: 'jog_release_all' }
  | { type: 'select_tool'; id: number }
  | { type: 'set_tools'; tools: Tool[] }
  | { type: 'set_preferences'; preferences: Preferences }
  | { type: 'plan_threading'; form: ThreadingForm }
  | { type: 'plan_turning'; form: TurningForm }
  | { type: 'start_cycle' }
  | { type: 'abort_cycle' }
  | { type: 'quit' }
