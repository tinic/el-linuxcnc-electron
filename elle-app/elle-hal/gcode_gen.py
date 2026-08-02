"""Pure G-code generators for Elle canned cycles.

This module has no dependencies on hal, linuxcnc, or Flask so it can be
imported and unit-tested on any machine. The REST layer
(lathe_halcomp.py) is responsible for wrapping the returned lines into an
o<canned-cycle> subroutine and handing them to LinuxCNC.

Both generators return G-code in radius mode (G8), metric units (G21),
absolute positioning (G90). When ``for_backplot`` is False the program
additionally sets the work coordinate system (G10 L20 / G54) from the
caller-supplied XPos/ZPos so execution happens in work coordinates.
"""

from __future__ import annotations

import math
from collections.abc import Mapping
from typing import Any

Params = Mapping[str, Any]


def generate_threading_gcode(params: Params, for_backplot: bool = False) -> list[str]:
    """Generate a G33 threading cycle.

    Required params: XStart, ZStart, Pitch, XDepth, ZDepth, XEnd, ZEnd,
    XPullout, ZPullout, FirstCut, CutMult, MinCut, SpringCuts, XReturn,
    ZReturn — plus XPos, ZPos when ``for_backplot`` is False.
    """
    x_start = float(params['XStart'])
    z_start = float(params['ZStart'])
    pitch = abs(float(params['Pitch']))
    x_depth = float(params['XDepth'])
    z_depth = float(params['ZDepth'])
    x_end = float(params['XEnd'])
    z_end = float(params['ZEnd'])
    x_pullout = float(params['XPullout'])
    z_pullout = float(params['ZPullout'])
    first_cut = abs(float(params['FirstCut']))
    cut_mult = abs(float(params['CutMult']))
    min_cut = abs(float(params['MinCut']))
    spring_cuts = int(params['SpringCuts'])
    x_return = float(params['XReturn'])
    z_return = float(params['ZReturn'])

    # Calculate compound distance and direction ratios
    compound_dist = math.sqrt(x_depth * x_depth + z_depth * z_depth)
    if compound_dist == 0:
        raise ValueError("XDepth and ZDepth are both zero; nothing to cut")
    if first_cut == 0 and min_cut == 0:
        raise ValueError("FirstCut and MinCut are both zero; cut would never advance")
    k_x = x_depth / compound_dist
    k_z = z_depth / compound_dist

    gcode_lines: list[str] = []

    # Common setup
    gcode_lines.append("G8")     # Radius mode
    gcode_lines.append("G21")    # Metric units
    gcode_lines.append("G90")    # Absolute positioning
    gcode_lines.append("F100")   # Set feed rate for G1 moves
    gcode_lines.append("M3S500") # Start spindle (required for G33)

    # Additional setup for execution (not backplot)
    if not for_backplot:
        gcode_lines.append(f"G10 L20 P1 X{float(params['XPos']):.6f} Z{float(params['ZPos']):.6f}")
        gcode_lines.append("G54")  # Use work coordinates

    # Move to start point
    gcode_lines.append(f"G0 X{x_start:.6f} Z{z_start:.6f}")

    # Threading loop state
    cut_size = 0.0
    x_cut = 0.0
    z_cut = 0.0
    spring_cuts_remaining = spring_cuts

    pass_number = 0
    while spring_cuts_remaining >= 0:
        pass_number += 1

        # Progressive cut sizing: first cut, then geometric reduction
        if cut_size == 0.0:
            cut_size = first_cut
        else:
            cut_size = cut_size * cut_mult

        # Never cut less than the minimum
        if abs(cut_size) < abs(min_cut):
            cut_size = min_cut

        # Advance along the compound direction
        x_cut = x_cut + (cut_size * k_x)
        z_cut = z_cut + (cut_size * k_z)

        # Clamp to full depth
        if abs(x_cut) >= abs(x_depth):
            x_cut = x_depth
            z_cut = z_depth

        gcode_lines.append(f"(Pass {pass_number} - Cut size: {cut_size:.4f})")

        # Move to cut start position
        cut_start_x = x_start + x_cut
        cut_start_z = z_start + z_cut
        gcode_lines.append(f"G1 X{cut_start_x:.6f} Z{cut_start_z:.6f}")

        # Dwell before the synchronized move (skipped for backplot)
        if not for_backplot:
            gcode_lines.append("G4 P0.01")

        # Spindle-synchronized threading pass
        cut_end_x = x_end + x_cut
        cut_end_z = z_end + z_cut
        gcode_lines.append(f"G33 X{cut_end_x:.6f} Z{cut_end_z:.6f} K{pitch:.6f}")

        # Pull out while still synchronized
        pullout_z = cut_end_z + z_pullout
        gcode_lines.append(f"G33 X{x_end:.6f} Z{pullout_z:.6f} K{pitch:.6f}")
        gcode_lines.append(f"G1 X{x_end:.6f} Z{pullout_z:.6f}")

        # Retract sequence
        retract_x = x_end + x_pullout
        gcode_lines.append(f"G0 X{retract_x:.6f}")
        gcode_lines.append(f"G0 Z{z_start:.6f}")
        gcode_lines.append(f"G0 X{x_start:.6f}")

        # Spring cut accounting: once at depth, repeat passes without advancing
        if abs(x_cut) == abs(x_depth):
            if spring_cuts_remaining > 0:
                x_cut = x_cut - (cut_size * k_x)
                z_cut = z_cut - (cut_size * k_z)
            spring_cuts_remaining -= 1

        if spring_cuts_remaining < 0:
            break

    # Final return to safe position
    gcode_lines.append(f"G0 X{x_return:.6f} Z{z_return:.6f}")

    return gcode_lines


def generate_turning_gcode(params: Params, for_backplot: bool = False) -> list[str]:
    """Generate a G33 turning (straight or taper) cycle.

    Required params: Pitch, Stock, Target, ZLead, ZEnd, Angle, StepDown,
    FinalStepDown, SpringPasses, XReturn, ZReturn — plus XPos, ZPos when
    ``for_backplot`` is False.
    """
    pitch = abs(float(params['Pitch']))        # Cutting pitch for G33
    x_stock = float(params['Stock'])           # Stock radius (starting)
    x_target = float(params['Target'])         # Target radius (finished)

    z_start = 0.0                              # Cutting always starts at Z=0
    z_lead = float(params['ZLead'])            # Lead-in, compensates backlash
    z_end = float(params['ZEnd'])              # Full cut depth, usually negative
    angle = float(params['Angle'])             # Taper angle in degrees
    step_down = float(params['StepDown'])      # Depth per roughing pass
    final_step_down = float(params['FinalStepDown'])  # Depth of finishing pass
    spring_passes = int(params['SpringPasses'])
    x_return = float(params['XReturn'])
    z_return = float(params['ZReturn'])

    gcode_lines: list[str] = []

    # Common setup
    gcode_lines.append("G8")     # Radius mode
    gcode_lines.append("G21")    # Metric units
    gcode_lines.append("G90")    # Absolute positioning
    gcode_lines.append("F100")   # Set feed rate
    gcode_lines.append("M3S500") # Start spindle

    # Additional setup for execution (not backplot)
    if not for_backplot:
        gcode_lines.append(f"G10 L20 P1 X{float(params['XPos']):.6f} Z{float(params['ZPos']):.6f}")
        gcode_lines.append("G54")  # Use work coordinates

    # Move to start point
    gcode_lines.append(f"G0 X{x_stock:.6f} Z{z_start:.6f}")

    angle_rad = math.radians(angle)

    # Pass planning: roughing passes down to the finishing allowance,
    # one final pass to target, then spring passes at target depth.
    total_cut_depth = abs(x_stock - x_target)
    remaining_after_final = total_cut_depth - final_step_down
    num_roughing_passes = 0
    if remaining_after_final > 0:
        if step_down <= 0:
            raise ValueError("StepDown must be positive when roughing passes are required")
        num_roughing_passes = int(math.ceil(remaining_after_final / step_down))

    passes: list[tuple[str, int, int, float]] = []
    for i in range(num_roughing_passes):
        depth = min((i + 1) * step_down, remaining_after_final)
        passes.append(("Roughing", i + 1, num_roughing_passes, depth))
    passes.append(("Final", 1, 1, total_cut_depth))
    for i in range(spring_passes):
        passes.append(("Spring", i + 1, spring_passes, total_cut_depth))

    z_travel = z_end - z_start

    # Retract clear of the largest diameter
    retract_x = x_stock + 2.0

    for pass_type, pass_num, total_of_type, depth in passes:
        if total_of_type > 1:
            gcode_lines.append(f"({pass_type} pass {pass_num} of {total_of_type})")
        else:
            gcode_lines.append(f"({pass_type} pass)")

        # Cut from stock inward by the cumulative depth of this pass
        cut_diameter = x_stock - depth

        # Taper compensation at the lead-in and end-of-cut Z positions
        adjusted_x_start = cut_diameter - (z_lead * math.tan(angle_rad))
        adjusted_x_end = cut_diameter - (z_travel * math.tan(angle_rad))

        gcode_lines.append(f"G0 X{adjusted_x_start:.6f} Z{z_lead:.6f}")
        gcode_lines.append(f"G33 X{adjusted_x_end:.6f} Z{z_end:.6f} K{pitch:.6f}")
        gcode_lines.append(f"G0 X{retract_x:.6f}")
        gcode_lines.append(f"G0 Z{z_start:.6f}")

    # Return to safe position
    gcode_lines.append(f"G0 X{x_return:.6f} Z{z_return:.6f}")

    return gcode_lines


def wrap_subroutine(gcode_lines: list[str], name: str = "canned-cycle") -> str:
    """Wrap generated lines in the o<name> subroutine format LinuxCNC expects."""
    body = "\n".join(gcode_lines)
    return f"o<{name}> sub\n{body}\no<{name}> endsub\n"
