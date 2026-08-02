"""Single source of truth for the Elle REST API contract.

Every request/response body exchanged between the Vue frontend and this
backend is defined here as a pydantic model. The TypeScript side of the
contract (elle-frontend/src/api/types.gen.ts) is generated from these
models via generate_schema.py — run `yarn generate:api` after changing
anything in this file. A pytest guard (test_api_schema.py) fails if the
committed schema drifts from these models.

Works with both pydantic v1 (Debian bookworm's python3-pydantic) and
pydantic v2 (Debian trixie and newer) via the small compat helpers at the
bottom.
"""

from __future__ import annotations

from typing import List, Optional

import pydantic
from pydantic import BaseModel

PYDANTIC_V2 = pydantic.VERSION.startswith("2")


class HalIn(BaseModel):
    """Position/status snapshot returned by GET /hal/hal_in (30 Hz poll)."""

    position_z: float
    position_x: float
    position_a: float
    speed_rps: float
    program_running: bool
    error_state: bool


class HalOut(BaseModel):
    """Control command for PUT /hal/hal_out.

    All fields are optional; the backend only acts on fields that are
    present in the request (presence semantics, not null semantics).
    """

    control_stop_now: Optional[int] = None
    reset_position: Optional[bool] = None
    encoder_scale_z: Optional[float] = None
    encoder_scale_x: Optional[float] = None
    control_z_type: Optional[int] = None
    control_x_type: Optional[int] = None
    velocity_z_cmd: Optional[float] = None
    velocity_x_cmd: Optional[float] = None
    control_source: Optional[bool] = None
    enable_stepper_z: Optional[bool] = None
    enable_stepper_x: Optional[bool] = None
    forward_z: Optional[float] = None
    forward_x: Optional[float] = None
    enable_z: Optional[bool] = None
    enable_x: Optional[bool] = None


class ThreadingParams(BaseModel):
    """Parameters for the G33 threading cycle (backplot generation)."""

    XStart: float
    ZStart: float
    Pitch: float
    XDepth: float
    ZDepth: float
    XEnd: float
    ZEnd: float
    XPullout: float
    ZPullout: float
    FirstCut: float
    CutMult: float
    MinCut: float
    SpringCuts: int
    XReturn: float
    ZReturn: float


class ThreadingExecuteParams(ThreadingParams):
    """Threading execution additionally needs the current work position."""

    XPos: float
    ZPos: float


class TurningParams(BaseModel):
    """Parameters for the G33 turning cycle (backplot generation)."""

    Pitch: float
    Stock: float
    Target: float
    ZLead: float
    ZEnd: float
    Angle: float
    StepDown: float
    FinalStepDown: float
    SpringPasses: int
    XReturn: float
    ZReturn: float


class TurningExecuteParams(TurningParams):
    """Turning execution additionally needs the current work position."""

    XPos: float
    ZPos: float


class StatusResponse(BaseModel):
    """Generic status envelope used by command endpoints."""

    status: str
    message: Optional[str] = None


class GcodeResponse(StatusResponse):
    """Response carrying generated G-code (generate + execute endpoints)."""

    gcode: List[str] = []
    subroutine_file: Optional[str] = None


class CleanupResponse(StatusResponse):
    files_removed: List[str] = []


ALL_MODELS = [
    HalIn,
    HalOut,
    ThreadingParams,
    ThreadingExecuteParams,
    TurningParams,
    TurningExecuteParams,
    StatusResponse,
    GcodeResponse,
    CleanupResponse,
]


class ApiValidationError(ValueError):
    """Raised when a request body does not match its model."""


def validate(model_cls, data):
    """Parse `data` into `model_cls`, raising ApiValidationError on failure."""
    if data is None:
        raise ApiValidationError(f"{model_cls.__name__}: request body is missing")
    try:
        if PYDANTIC_V2:
            return model_cls.model_validate(data)
        return model_cls.parse_obj(data)
    except pydantic.ValidationError as e:
        raise ApiValidationError(f"{model_cls.__name__}: {e}") from e


def dump(instance, exclude_unset: bool = False) -> dict:
    """Model → plain dict, optionally only fields present in the request."""
    if PYDANTIC_V2:
        return instance.model_dump(exclude_unset=exclude_unset)
    return instance.dict(exclude_unset=exclude_unset)


def _strip_property_titles(schema: dict) -> None:
    """Remove per-property titles so codegen emits plain types, not aliases."""
    for prop in schema.get("properties", {}).values():
        if isinstance(prop, dict):
            prop.pop("title", None)


def json_schema() -> dict:
    """JSON Schema for all models, keyed under $defs (used by codegen)."""
    defs = {}
    for model in ALL_MODELS:
        if PYDANTIC_V2:
            schema = model.model_json_schema(
                ref_template="#/$defs/{model}", mode="serialization"
            )
        else:
            schema = model.schema(ref_template="#/$defs/{model}")
        nested = schema.pop("$defs", schema.pop("definitions", {}) or {})
        for sub in nested.values():
            if isinstance(sub, dict):
                _strip_property_titles(sub)
        defs.update(nested)
        _strip_property_titles(schema)
        defs[model.__name__] = schema
    return {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "title": "ElleApi",
        "$defs": defs,
    }
