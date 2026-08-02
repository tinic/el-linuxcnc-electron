/* eslint-disable */
/* AUTO-GENERATED from api-schema.json - do not edit. Run: yarn generate:api */

export type FilesRemoved = string[];
export type Message = string | null;
export type Status = string;
export type Gcode = string[];
export type Message1 = string | null;
export type Status1 = string;
export type SubroutineFile = string | null;
export type ErrorState = boolean;
export type PositionA = number;
export type PositionX = number;
export type PositionZ = number;
export type ProgramRunning = boolean;
export type SpeedRps = number;
export type ControlSource = boolean | null;
export type ControlStopNow = number | null;
export type ControlXType = number | null;
export type ControlZType = number | null;
export type EnableStepperX = boolean | null;
export type EnableStepperZ = boolean | null;
export type EnableX = boolean | null;
export type EnableZ = boolean | null;
export type EncoderScaleX = number | null;
export type EncoderScaleZ = number | null;
export type ForwardX = number | null;
export type ForwardZ = number | null;
export type ResetPosition = boolean | null;
export type VelocityXCmd = number | null;
export type VelocityZCmd = number | null;
export type Message2 = string | null;
export type Status2 = string;
export type Cutmult = number;
export type Firstcut = number;
export type Mincut = number;
export type Pitch = number;
export type Springcuts = number;
export type Xdepth = number;
export type Xend = number;
export type Xpos = number;
export type Xpullout = number;
export type Xreturn = number;
export type Xstart = number;
export type Zdepth = number;
export type Zend = number;
export type Zpos = number;
export type Zpullout = number;
export type Zreturn = number;
export type Zstart = number;
export type Cutmult1 = number;
export type Firstcut1 = number;
export type Mincut1 = number;
export type Pitch1 = number;
export type Springcuts1 = number;
export type Xdepth1 = number;
export type Xend1 = number;
export type Xpullout1 = number;
export type Xreturn1 = number;
export type Xstart1 = number;
export type Zdepth1 = number;
export type Zend1 = number;
export type Zpullout1 = number;
export type Zreturn1 = number;
export type Zstart1 = number;
export type Angle = number;
export type Finalstepdown = number;
export type Pitch2 = number;
export type Springpasses = number;
export type Stepdown = number;
export type Stock = number;
export type Target = number;
export type Xpos1 = number;
export type Xreturn2 = number;
export type Zend2 = number;
export type Zlead = number;
export type Zpos1 = number;
export type Zreturn2 = number;
export type Angle1 = number;
export type Finalstepdown1 = number;
export type Pitch3 = number;
export type Springpasses1 = number;
export type Stepdown1 = number;
export type Stock1 = number;
export type Target1 = number;
export type Xreturn3 = number;
export type Zend3 = number;
export type Zlead1 = number;
export type Zreturn3 = number;

export interface ElleApi {
  [k: string]: unknown;
}
/**
 * This interface was referenced by `ElleApi`'s JSON-Schema
 * via the `definition` "CleanupResponse".
 */
export interface CleanupResponse {
  files_removed?: FilesRemoved;
  message?: Message;
  status: Status;
  [k: string]: unknown;
}
/**
 * Response carrying generated G-code (generate + execute endpoints).
 *
 * This interface was referenced by `ElleApi`'s JSON-Schema
 * via the `definition` "GcodeResponse".
 */
export interface GcodeResponse {
  gcode?: Gcode;
  message?: Message1;
  status: Status1;
  subroutine_file?: SubroutineFile;
  [k: string]: unknown;
}
/**
 * Position/status snapshot returned by GET /hal/hal_in (30 Hz poll).
 *
 * This interface was referenced by `ElleApi`'s JSON-Schema
 * via the `definition` "HalIn".
 */
export interface HalIn {
  error_state: ErrorState;
  position_a: PositionA;
  position_x: PositionX;
  position_z: PositionZ;
  program_running: ProgramRunning;
  speed_rps: SpeedRps;
  [k: string]: unknown;
}
/**
 * Control command for PUT /hal/hal_out.
 *
 * All fields are optional; the backend only acts on fields that are
 * present in the request (presence semantics, not null semantics).
 *
 * This interface was referenced by `ElleApi`'s JSON-Schema
 * via the `definition` "HalOut".
 */
export interface HalOut {
  control_source?: ControlSource;
  control_stop_now?: ControlStopNow;
  control_x_type?: ControlXType;
  control_z_type?: ControlZType;
  enable_stepper_x?: EnableStepperX;
  enable_stepper_z?: EnableStepperZ;
  enable_x?: EnableX;
  enable_z?: EnableZ;
  encoder_scale_x?: EncoderScaleX;
  encoder_scale_z?: EncoderScaleZ;
  forward_x?: ForwardX;
  forward_z?: ForwardZ;
  reset_position?: ResetPosition;
  velocity_x_cmd?: VelocityXCmd;
  velocity_z_cmd?: VelocityZCmd;
  [k: string]: unknown;
}
/**
 * Generic status envelope used by command endpoints.
 *
 * This interface was referenced by `ElleApi`'s JSON-Schema
 * via the `definition` "StatusResponse".
 */
export interface StatusResponse {
  message?: Message2;
  status: Status2;
  [k: string]: unknown;
}
/**
 * Threading execution additionally needs the current work position.
 *
 * This interface was referenced by `ElleApi`'s JSON-Schema
 * via the `definition` "ThreadingExecuteParams".
 */
export interface ThreadingExecuteParams {
  CutMult: Cutmult;
  FirstCut: Firstcut;
  MinCut: Mincut;
  Pitch: Pitch;
  SpringCuts: Springcuts;
  XDepth: Xdepth;
  XEnd: Xend;
  XPos: Xpos;
  XPullout: Xpullout;
  XReturn: Xreturn;
  XStart: Xstart;
  ZDepth: Zdepth;
  ZEnd: Zend;
  ZPos: Zpos;
  ZPullout: Zpullout;
  ZReturn: Zreturn;
  ZStart: Zstart;
  [k: string]: unknown;
}
/**
 * Parameters for the G33 threading cycle (backplot generation).
 *
 * This interface was referenced by `ElleApi`'s JSON-Schema
 * via the `definition` "ThreadingParams".
 */
export interface ThreadingParams {
  CutMult: Cutmult1;
  FirstCut: Firstcut1;
  MinCut: Mincut1;
  Pitch: Pitch1;
  SpringCuts: Springcuts1;
  XDepth: Xdepth1;
  XEnd: Xend1;
  XPullout: Xpullout1;
  XReturn: Xreturn1;
  XStart: Xstart1;
  ZDepth: Zdepth1;
  ZEnd: Zend1;
  ZPullout: Zpullout1;
  ZReturn: Zreturn1;
  ZStart: Zstart1;
  [k: string]: unknown;
}
/**
 * Turning execution additionally needs the current work position.
 *
 * This interface was referenced by `ElleApi`'s JSON-Schema
 * via the `definition` "TurningExecuteParams".
 */
export interface TurningExecuteParams {
  Angle: Angle;
  FinalStepDown: Finalstepdown;
  Pitch: Pitch2;
  SpringPasses: Springpasses;
  StepDown: Stepdown;
  Stock: Stock;
  Target: Target;
  XPos: Xpos1;
  XReturn: Xreturn2;
  ZEnd: Zend2;
  ZLead: Zlead;
  ZPos: Zpos1;
  ZReturn: Zreturn2;
  [k: string]: unknown;
}
/**
 * Parameters for the G33 turning cycle (backplot generation).
 *
 * This interface was referenced by `ElleApi`'s JSON-Schema
 * via the `definition` "TurningParams".
 */
export interface TurningParams {
  Angle: Angle1;
  FinalStepDown: Finalstepdown1;
  Pitch: Pitch3;
  SpringPasses: Springpasses1;
  StepDown: Stepdown1;
  Stock: Stock1;
  Target: Target1;
  XReturn: Xreturn3;
  ZEnd: Zend3;
  ZLead: Zlead1;
  ZReturn: Zreturn3;
  [k: string]: unknown;
}
