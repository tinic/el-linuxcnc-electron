"""Characterization and invariant tests for the canned-cycle G-code generators.

The golden files in tests/golden/ were generated from the original inline
implementation in lathe_halcomp.py (verified byte-identical at extraction
time). If a test here fails, either a regression was introduced or the
generator behavior was changed intentionally — in the latter case inspect
the diff carefully and regenerate the golden files.
"""

import json
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from gcode_gen import (  # noqa: E402
    generate_threading_gcode,
    generate_turning_gcode,
    wrap_subroutine,
)

HERE = Path(__file__).resolve().parent
GOLDEN = HERE / "golden"
CASES = json.loads((HERE / "cases.json").read_text())

GENERATORS = {
    "threading": generate_threading_gcode,
    "turning": generate_turning_gcode,
}


def golden_ids():
    for kind, cases in CASES.items():
        for name in cases:
            for mode in ("backplot", "execute"):
                yield kind, name, mode


@pytest.mark.parametrize("kind,name,mode", list(golden_ids()),
                         ids=lambda v: v if isinstance(v, str) else str(v))
def test_matches_golden(kind, name, mode):
    params = CASES[kind][name]
    lines = GENERATORS[kind](params, for_backplot=(mode == "backplot"))
    expected = (GOLDEN / f"{name}.{mode}.ngc").read_text().splitlines()
    assert lines == expected


@pytest.mark.parametrize("kind,name", [(k, n) for k, c in CASES.items() for n in c])
def test_execute_mode_sets_work_coordinates(kind, name):
    params = CASES[kind][name]
    execute = GENERATORS[kind](params, for_backplot=False)
    backplot = GENERATORS[kind](params, for_backplot=True)
    assert any(line.startswith("G10 L20 P1") for line in execute)
    assert "G54" in execute
    assert not any(line.startswith("G10") for line in backplot)
    assert "G54" not in backplot


@pytest.mark.parametrize("kind,name", [(k, n) for k, c in CASES.items() for n in c])
def test_common_invariants(kind, name):
    params = CASES[kind][name]
    lines = GENERATORS[kind](params, for_backplot=True)

    # Modal setup comes first: radius mode, metric, absolute
    assert lines[:3] == ["G8", "G21", "G90"]

    # Spindle must be commanded on before any synchronized move
    m3_index = lines.index("M3S500")
    first_g33 = next(i for i, l in enumerate(lines) if l.startswith("G33"))
    assert m3_index < first_g33

    # Every synchronized move carries an explicit pitch word
    for line in lines:
        if line.startswith("G33"):
            assert " K" in line, f"G33 without pitch: {line}"

    # Program ends at the caller-requested safe position
    assert lines[-1] == (
        f"G0 X{float(params['XReturn']):.6f} Z{float(params['ZReturn']):.6f}"
    )


def test_threading_reaches_full_depth():
    params = CASES["threading"]["thread_metric_m10x1.5"]
    lines = generate_threading_gcode(params, for_backplot=True)
    x_depth = float(params["XDepth"])
    x_end = float(params["XEnd"])
    # The deepest G33 X endpoint must equal XEnd + XDepth (full thread depth)
    g33_x = [float(l.split()[1][1:]) for l in lines if l.startswith("G33 X")]
    assert min(g33_x) == pytest.approx(x_end + x_depth, abs=1e-6)


def test_turning_pass_plan():
    params = CASES["turning"]["turn_straight"]
    lines = generate_turning_gcode(params, for_backplot=True)
    total_depth = abs(float(params["Stock"]) - float(params["Target"]))
    roughing = [l for l in lines if l.startswith("(Roughing")]
    final = [l for l in lines if l.startswith("(Final")]
    spring = [l for l in lines if l.startswith("(Spring")]
    assert len(final) == 1
    assert len(spring) == int(params["SpringPasses"])
    # Roughing must leave exactly the finishing allowance
    import math
    expected_roughing = math.ceil(
        (total_depth - float(params["FinalStepDown"])) / float(params["StepDown"])
    )
    assert len(roughing) == expected_roughing


def test_threading_rejects_zero_depth():
    params = dict(CASES["threading"]["thread_metric_m10x1.5"])
    params["XDepth"] = 0.0
    params["ZDepth"] = 0.0
    with pytest.raises(ValueError):
        generate_threading_gcode(params, for_backplot=True)


def test_threading_rejects_non_advancing_cut():
    """FirstCut=0 with MinCut=0 used to hang the Flask handler forever."""
    params = dict(CASES["threading"]["thread_metric_m10x1.5"])
    params["FirstCut"] = 0.0
    params["MinCut"] = 0.0
    with pytest.raises(ValueError):
        generate_threading_gcode(params, for_backplot=True)


def test_turning_rejects_zero_step_down():
    params = dict(CASES["turning"]["turn_straight"])
    params["StepDown"] = 0.0
    with pytest.raises(ValueError):
        generate_turning_gcode(params, for_backplot=True)


def test_wrap_subroutine_format():
    wrapped = wrap_subroutine(["G0 X1.000000", "G0 Z0.000000"])
    assert wrapped == (
        "o<canned-cycle> sub\n"
        "G0 X1.000000\n"
        "G0 Z0.000000\n"
        "o<canned-cycle> endsub\n"
    )
