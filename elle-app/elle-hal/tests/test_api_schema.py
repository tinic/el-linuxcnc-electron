"""Guards against drift between api_models.py and the committed schema.

If this fails you changed the API models without regenerating the
contract: run `python3 elle-hal/generate_schema.py` and `yarn generate:api`
in elle-frontend, then commit the results.
"""

import json
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from api_models import (  # noqa: E402
    ApiValidationError,
    HalOut,
    ThreadingExecuteParams,
    ThreadingParams,
    dump,
    json_schema,
    validate,
)

SCHEMA_FILE = Path(__file__).resolve().parent.parent.parent / "api-schema.json"


def test_committed_schema_is_current():
    committed = json.loads(SCHEMA_FILE.read_text())
    assert committed == json_schema(), (
        "api-schema.json is stale — run generate_schema.py and yarn generate:api"
    )


def test_hal_out_preserves_presence_semantics():
    body = {"enable_z": True, "velocity_z_cmd": 1.5}
    parsed = dump(validate(HalOut, body), exclude_unset=True)
    assert parsed == {"enable_z": True, "velocity_z_cmd": 1.5}
    assert "reset_position" not in parsed  # absent fields must stay absent


def test_threading_params_reject_missing_fields():
    with pytest.raises(ApiValidationError):
        validate(ThreadingParams, {"XStart": 1.0})


def test_threading_params_reject_wrong_types():
    good = dict(XStart=5.2, ZStart=2.0, Pitch=1.5, XDepth=-0.92, ZDepth=0.0,
                XEnd=5.2, ZEnd=-20.0, XPullout=1.0, ZPullout=1.5, FirstCut=0.25,
                CutMult=0.8, MinCut=0.05, SpringCuts=2, XReturn=8.0, ZReturn=5.0)
    validate(ThreadingParams, good)  # sanity: valid body passes
    bad = dict(good, Pitch="fast")
    with pytest.raises(ApiValidationError):
        validate(ThreadingParams, bad)


def test_execute_params_require_work_position():
    body = dict(XStart=5.2, ZStart=2.0, Pitch=1.5, XDepth=-0.92, ZDepth=0.0,
                XEnd=5.2, ZEnd=-20.0, XPullout=1.0, ZPullout=1.5, FirstCut=0.25,
                CutMult=0.8, MinCut=0.05, SpringCuts=2, XReturn=8.0, ZReturn=5.0)
    validate(ThreadingParams, body)
    with pytest.raises(ApiValidationError):
        validate(ThreadingExecuteParams, body)  # XPos/ZPos missing


def test_missing_body_is_rejected():
    with pytest.raises(ApiValidationError):
        validate(HalOut, None)
