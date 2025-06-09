#!/usr/bin/env python3
import hal
import sys
import linuxcnc
import time
import os
import tempfile

from flask import Flask
from flask_cors import CORS
from flask import request

halc = hal.component("lathe")
haluic = hal.component("halui")
c = linuxcnc.command()

hal_pin_machine_is_on = haluic.newpin("machine.is-on", hal.HAL_BIT, hal.HAL_OUT)

hal_pin_control_source = halc.newpin("control_source", hal.HAL_BIT, hal.HAL_OUT)

hal_pin_position_z = halc.newpin("position_z", hal.HAL_FLOAT, hal.HAL_IN)
hal_pin_position_x = halc.newpin("position_x", hal.HAL_FLOAT, hal.HAL_IN)
hal_pin_position_a = halc.newpin("position_a", hal.HAL_FLOAT, hal.HAL_IN)
hal_pin_speed_rps = halc.newpin("speed_rps", hal.HAL_FLOAT, hal.HAL_IN)

hal_pin_forward_z = halc.newpin("forward_z", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_forward_x = halc.newpin("forward_x", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_enable_z = halc.newpin("enable_z", hal.HAL_BIT, hal.HAL_OUT)
hal_pin_enable_x = halc.newpin("enable_x", hal.HAL_BIT, hal.HAL_OUT)
hal_pin_enable_stepper_z = halc.newpin("enable_stepper_z", hal.HAL_BIT, hal.HAL_OUT)
hal_pin_enable_stepper_x = halc.newpin("enable_stepper_x", hal.HAL_BIT, hal.HAL_OUT)

hal_pin_offset_z_encoder = halc.newpin("offset_z_encoder", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_offset_z_stepper = halc.newpin("offset_z_stepper", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_offset_x_encoder = halc.newpin("offset_x_encoder", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_offset_x_stepper = halc.newpin("offset_x_stepper", hal.HAL_FLOAT, hal.HAL_OUT)

hal_pin_control_z_type = halc.newpin("control_z_type", hal.HAL_BIT, hal.HAL_OUT)
hal_pin_control_x_type = halc.newpin("control_x_type", hal.HAL_BIT, hal.HAL_OUT)
hal_pin_velocity_z_cmd = halc.newpin("velocity_z_cmd", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_velocity_x_cmd = halc.newpin("velocity_x_cmd", hal.HAL_FLOAT, hal.HAL_OUT)

app = Flask(__name__)
CORS(app, resources={r"/*": {"origins": "*"}})


@app.route("/")
def index():
    return {"status": "OK!"}


@app.get("/hal/hal_in")
def read_hal_in():
    s = linuxcnc.stat()
    s.poll()
    
    error_state = (
        s.estop or                              # E-stop active
        s.exec_state == linuxcnc.EXEC_ERROR     # Execution error
    )
    
    program_running = (
        s.interp_state != linuxcnc.INTERP_IDLE or
        s.exec_state in [linuxcnc.EXEC_WAITING_FOR_MOTION, 
                        linuxcnc.EXEC_WAITING_FOR_MOTION_QUEUE, 
                        linuxcnc.EXEC_WAITING_FOR_IO] or
        s.call_level > 0
    )
    
    return {
        "position_z": hal_pin_position_z.get(),
        "position_x": hal_pin_position_x.get(),
        "position_a": hal_pin_position_a.get(),
        "speed_rps": hal_pin_speed_rps.get(),
        "program_running": program_running,
        "error_state": error_state
    }

@app.put("/hal/abort")
def abort_operation():
    try:
        # Abort current operation without E-stop
        c.abort()
        
        
        return {"status": "OK", "message": "Operation aborted"}
        
    except Exception as e:
        error_msg = f"Error during abort: {str(e)}"
        return {"status": "Error", "message": error_msg}, 500

@app.put("/hal/estop")
def emergency_stop():
    try:
        # Immediate abort of all operations
        c.abort()
        
        # Set machine to E-stop state
        c.state(linuxcnc.STATE_ESTOP)
        
        
        return {"status": "OK", "message": "Emergency stop executed"}
        
    except Exception as e:
        error_msg = f"Error during emergency stop: {str(e)}"
        return {"status": "Error", "message": error_msg}, 500


def generate_threading_gcode_core(params, for_backplot=False):
    import math
    
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
    k_x = x_depth / compound_dist if compound_dist != 0 else 0
    k_z = z_depth / compound_dist if compound_dist != 0 else 0
    
    gcode_lines = []
    
    # Common setup
    gcode_lines.append("G8")   # Radius mode
    gcode_lines.append("G21")  # Metric units
    gcode_lines.append("G90")  # Absolute positioning
    gcode_lines.append("F100") # Set feed rate for G1 moves
    gcode_lines.append("M3S500") # Start spindle (required for G33)
    
    # Additional setup for execution (not backplot)
    if not for_backplot:
        gcode_lines.append(f"G10 L20 P1 X{float(params['XPos']):.6f} Z{float(params['ZPos']):.6f}")
        gcode_lines.append("G54")  # Use work coordinates
    
    # Move to start point (line 40)
    gcode_lines.append(f"G0 X{x_start:.6f} Z{z_start:.6f}")
    
    # Threading loop variables
    cut_size = 0.0
    x_cut = 0.0
    z_cut = 0.0
    spring_cuts_remaining = spring_cuts
    
    # Main threading loop (o100 do ... o100 while from lines 41-77)
    pass_number = 0
    while spring_cuts_remaining >= 0:
        pass_number += 1
        
        # Calculate cut size (lines 42-47)
        if cut_size == 0.0:
            cut_size = first_cut
        else:
            cut_size = cut_size * cut_mult
            
        # Apply minimum cut constraint (lines 49-52)
        if abs(cut_size) < abs(min_cut):
            cut_size = min_cut
            
        # Calculate cut positions (lines 53-54)
        x_cut = x_cut + (cut_size * k_x)
        z_cut = z_cut + (cut_size * k_z)
        
        # Don't go too far (lines 56-59)
        if abs(x_cut) >= abs(x_depth):
            x_cut = x_depth
            z_cut = z_depth
            
        # Threading pass
        gcode_lines.append(f"(Pass {pass_number} - Cut size: {cut_size:.4f})")
        
        # Move to cut start position (line 60)
        cut_start_x = x_start + x_cut
        cut_start_z = z_start + z_cut
        gcode_lines.append(f"G1 X{cut_start_x:.6f} Z{cut_start_z:.6f}")
        
        # Dwell (line 61) - Skip for backplot compatibility
        if not for_backplot:
            gcode_lines.append("G4 P0.01")
        
        # Cut thread (line 62)
        cut_end_x = x_end + x_cut
        cut_end_z = z_end + z_cut
        gcode_lines.append(f"G33 X{cut_end_x:.6f} Z{cut_end_z:.6f} K{pitch:.6f}")
        
        # Pull out (line 63)
        pullout_z = cut_end_z + z_pullout
        gcode_lines.append(f"G33 X{x_end:.6f} Z{pullout_z:.6f} K{pitch:.6f}")
        
        # Continue pullout (line 66)
        gcode_lines.append(f"G1 X{x_end:.6f} Z{pullout_z:.6f}")
        
        # Retract sequence (lines 67-69)
        retract_x = x_end + x_pullout
        gcode_lines.append(f"G0 X{retract_x:.6f}")
        gcode_lines.append(f"G0 Z{z_start:.6f}")
        gcode_lines.append(f"G0 X{x_start:.6f}")
        
        # Spring cut logic (lines 70-76)
        if abs(x_cut) == abs(x_depth):
            if spring_cuts_remaining > 0:
                # Back off for spring cut
                x_cut = x_cut - (cut_size * k_x)
                z_cut = z_cut - (cut_size * k_z)
            spring_cuts_remaining -= 1
            
        # Break if we've completed all cuts including spring cuts
        if spring_cuts_remaining < 0:
            break
    
    # Final return to safe position (line 78)
    gcode_lines.append(f"G0 X{x_return:.6f} Z{z_return:.6f}")
    
    return gcode_lines


def generate_turning_gcode_core(params, for_backplot=False):
    import math
    
    # Check if this is NPT threading (has taper angle ~1.79 degrees)
    taper_angle = float(params.get('TaperAngle', 0))
    is_npt_thread = abs(taper_angle - 1.79) < 0.1  # NPT threads have 1.79° taper
    
    if is_npt_thread:
        return generate_npt_turning_gcode(params, for_backplot)
    
    # Original turning logic for non-NPT operations
    x_start = float(params['XStart'])
    x_end = float(params['XEnd'])
    z_start = float(params['ZStart'])
    z_end = float(params['ZEnd'])
    feed_rate = float(params['FeedRate'])
    step_down = float(params['StepDown'])
    roughing_passes = int(params['RoughingPasses'])
    finishing_allowance = float(params['FinishingAllowance'])
    x_return = float(params['XReturn'])
    z_return = float(params['ZReturn'])
    
    gcode_lines = []
    
    # Common setup
    gcode_lines.append("G8")   # Radius mode
    gcode_lines.append("G21")  # Metric units
    gcode_lines.append("G90")  # Absolute positioning
    gcode_lines.append(f"F{feed_rate}")  # Set feed rate
    gcode_lines.append("M3S500") # Start spindle
    
    # Additional setup for execution (not backplot)
    if not for_backplot:
        gcode_lines.append(f"G10 L20 P1 X{float(params['XPos']):.6f} Z{float(params['ZPos']):.6f}")
        gcode_lines.append("G54")  # Use work coordinates
    
    # Move to start point
    gcode_lines.append(f"G0 X{x_start:.6f} Z{z_start:.6f}")
    
    # Calculate total material to remove
    total_x_removal = abs(x_end - x_start)
    z_length = abs(z_end - z_start)
    
    # Calculate taper ratio (X change per Z unit)
    taper_ratio = 0.0
    if z_length > 0:
        taper_ratio = (x_end - x_start) / (z_end - z_start)
    
    # Roughing passes
    for pass_num in range(1, roughing_passes + 1):
        gcode_lines.append(f"(Roughing pass {pass_num})")
        
        # Calculate current depth (leave finishing allowance on final pass)
        if pass_num == roughing_passes:
            current_x = x_end + (finishing_allowance if x_end < x_start else -finishing_allowance)
        else:
            depth_fraction = pass_num / roughing_passes
            current_x = x_start + (total_x_removal - finishing_allowance) * depth_fraction * (1 if x_end > x_start else -1)
        
        # Calculate start and end positions for this pass
        if abs(taper_ratio) > 0.0001:  # Has taper
            # For tapered cuts, offset the entire taper line by the current depth
            depth_offset = current_x - x_start
            current_x_start = x_start + depth_offset
            current_x_end = x_end + depth_offset
            current_z_start = z_start
            current_z_end = z_end
        else:  # Straight cut
            current_x_start = current_x
            current_x_end = current_x
            current_z_start = z_start
            current_z_end = z_end
        
        # Cut along the taper line (or straight if no taper)
        gcode_lines.append(f"G1 X{current_x_start:.6f} Z{current_z_start:.6f}")
        gcode_lines.append(f"G1 X{current_x_end:.6f} Z{current_z_end:.6f}")
        
        # Retract - determine pullout direction
        if abs(taper_ratio) > 0.0001:  # Has taper
            # Pull out perpendicular to the taper or along Z if very shallow
            if abs(taper_ratio) < 0.1:  # Very shallow taper - pull out in Z
                gcode_lines.append(f"G0 Z{z_start:.6f}")
                gcode_lines.append(f"G0 X{x_start:.6f}")
            else:  # Steeper taper - pull out perpendicular
                retract_x = x_start
                gcode_lines.append(f"G0 X{retract_x:.6f}")
                gcode_lines.append(f"G0 Z{z_start:.6f}")
        else:  # Straight cut
            gcode_lines.append(f"G0 X{x_start:.6f}")
            gcode_lines.append(f"G0 Z{z_start:.6f}")
    
    # Finishing pass
    if finishing_allowance > 0:
        gcode_lines.append("(Finishing pass)")
        
        # Cut to final position following taper line
        gcode_lines.append(f"G1 X{x_end:.6f} Z{z_end:.6f}")
    
    # Return to safe position
    gcode_lines.append(f"G0 X{x_return:.6f} Z{z_return:.6f}")
    
    return gcode_lines


def generate_npt_turning_gcode(params, for_backplot=False):
    """Generate proper NPT turning G-code using the complete preset specifications"""
    import math
    
    # Extract basic parameters
    z_start = float(params['ZStart'])
    z_end = float(params['ZEnd'])
    feed_rate = float(params['FeedRate'])
    roughing_passes = int(params['RoughingPasses'])
    finishing_allowance = float(params['FinishingAllowance'])
    x_return = float(params['XReturn'])
    z_return = float(params['ZReturn'])
    
    # Determine if external or internal based on preset name
    preset_name = params.get('PresetName', '')
    is_external = '-2A' in preset_name
    is_internal = '-2B' in preset_name
    
    gcode_lines = []
    
    # Common setup
    gcode_lines.append("G8")   # Radius mode
    gcode_lines.append("G21")  # Metric units
    gcode_lines.append("G90")  # Absolute positioning
    gcode_lines.append(f"F{feed_rate}")  # Set feed rate
    gcode_lines.append("M3S500") # Start spindle
    
    # Additional setup for execution (not backplot)
    if not for_backplot:
        gcode_lines.append(f"G10 L20 P1 X{float(params['XPos']):.6f} Z{float(params['ZPos']):.6f}")
        gcode_lines.append("G54")  # Use work coordinates
    
    # Get the actual taper start and end positions
    x_start = float(params['XStart'])
    x_end = float(params['XEnd'])
    
    if is_external:
        # External NPT: Cut from round stock to create external taper
        stock_diameter = float(params.get('StockDiameter', 20.0))
        cutting_depth = float(params.get('CuttingDepth', 2.72))
        
        gcode_lines.append(f"(External NPT - Stock: {stock_diameter:.1f}mm, Taper: {x_start:.3f} to {x_end:.3f})")
        
        # Calculate taper geometry from actual coordinates
        thread_length = abs(z_end - z_start)
        taper_per_length = (x_end - x_start) / thread_length
        
        # Start from safe position outside stock
        safe_x = stock_diameter / 2 + 2.0
        gcode_lines.append(f"G0 X{safe_x:.6f} Z{z_start:.6f}")
        
        # Calculate cutting passes
        total_depth = (stock_diameter / 2) - x_start
        
        for pass_num in range(1, roughing_passes + 1):
            gcode_lines.append(f"(External pass {pass_num})")
            
            # Calculate depth for this pass
            if pass_num == roughing_passes:
                current_depth = total_depth - finishing_allowance
            else:
                current_depth = (pass_num / roughing_passes) * (total_depth - finishing_allowance)
            
            # Calculate X positions along taper
            x_start_pass = stock_diameter / 2 - current_depth
            x_end_pass = x_start_pass + (thread_length * taper_per_length)
            
            # Cut the taper
            gcode_lines.append(f"G1 X{x_start_pass:.6f} Z{z_start:.6f}")
            gcode_lines.append(f"G1 X{x_end_pass:.6f} Z{z_end:.6f}")
            
            # Retract
            gcode_lines.append(f"G0 X{safe_x:.6f}")
            gcode_lines.append(f"G0 Z{z_start:.6f}")
        
        # Finishing pass
        if finishing_allowance > 0:
            gcode_lines.append("(External finishing pass)")
            gcode_lines.append(f"G1 X{x_start:.6f} Z{z_start:.6f}")
            gcode_lines.append(f"G1 X{x_end:.6f} Z{z_end:.6f}")
    
    elif is_internal:
        # Internal NPT: Bore from drilled hole to create internal taper
        drill_diameter = float(params.get('DrillDiameter', 11.11))
        boring_depth = float(params.get('BoringDepth', 2.605))
        
        gcode_lines.append(f"(Internal NPT - Drill: {drill_diameter:.2f}mm, Taper: {x_start:.3f} to {x_end:.3f})")
        
        # Calculate taper geometry from actual coordinates
        thread_length = abs(z_end - z_start)
        taper_per_length = (x_end - x_start) / thread_length
        
        # Start from drilled hole size
        start_x = drill_diameter / 2
        gcode_lines.append(f"G0 X{start_x:.6f} Z{z_start:.6f}")
        
        for pass_num in range(1, roughing_passes + 1):
            gcode_lines.append(f"(Internal pass {pass_num})")
            
            # Calculate depth for this pass
            if pass_num == roughing_passes:
                current_depth = boring_depth - finishing_allowance
            else:
                current_depth = (pass_num / roughing_passes) * (boring_depth - finishing_allowance)
            
            # Calculate X positions along taper
            x_start_pass = drill_diameter / 2 + current_depth
            x_end_pass = x_start_pass + (thread_length * taper_per_length)
            
            # Cut the internal taper
            gcode_lines.append(f"G1 X{x_start_pass:.6f} Z{z_start:.6f}")
            gcode_lines.append(f"G1 X{x_end_pass:.6f} Z{z_end:.6f}")
            
            # Retract
            gcode_lines.append(f"G0 X{start_x:.6f}")
            gcode_lines.append(f"G0 Z{z_start:.6f}")
        
        # Finishing pass
        if finishing_allowance > 0:
            gcode_lines.append("(Internal finishing pass)")
            gcode_lines.append(f"G1 X{x_start:.6f} Z{z_start:.6f}")
            gcode_lines.append(f"G1 X{x_end:.6f} Z{z_end:.6f}")
    
    else:
        # Fallback to basic taper if not clearly external or internal
        gcode_lines.append("(Basic taper - unknown NPT type)")
        x_start = float(params['XStart'])
        x_end = float(params['XEnd'])
        gcode_lines.append(f"G0 X{x_start:.6f} Z{z_start:.6f}")
        gcode_lines.append(f"G1 X{x_end:.6f} Z{z_end:.6f}")
    
    # Return to safe position
    gcode_lines.append(f"G0 X{x_return:.6f} Z{z_return:.6f}")
    
    return gcode_lines

@app.put("/hal/turning/generate")
def generate_turning():
    json_data = request.json
    
    if not json_data:
        return {"status": "Error", "message": "Missing turning parameters"}, 400

    try:
        gcode_lines = generate_turning_gcode_core(json_data, for_backplot=True)
        return {
            "status": "OK", 
            "message": "Turning G-code generated",
            "gcode": gcode_lines
        }
        
    except Exception as e:
        error_msg = f"Error generating turning G-code: {str(e)}"
        return {"status": "Error", "message": error_msg}, 500


@app.put("/hal/turning")
def execute_turning():
    json_data = request.json
    
    if not json_data:
        return {"status": "Error", "message": "Missing turning parameters"}, 400

    c.state(linuxcnc.STATE_ON)
    c.wait_complete()

    try:
        s = linuxcnc.stat()
        while True:
            s.poll()
            if s.estop:
                return {"status": "Error", "message": "Machine is in ESTOP state"}, 400
            if not s.enabled:
                return {"status": "Error", "message": "Machine is not enabled"}, 400
            if not s.homed:
                return {"status": "Error", "message": "Machine is not homed"}, 400
            if s.interp_state != linuxcnc.INTERP_IDLE:
                return {"status": "Error", "message": "Interpreter is not idle"}, 400
            if s.task_mode != linuxcnc.MODE_MDI:
                c.mode(linuxcnc.MODE_MDI)
                time.sleep(0.1)
                continue
            break
        c.wait_complete()

        gcode_lines = generate_turning_gcode_core(json_data, for_backplot=False)
        
        ngc_filename = "canned-cycle.ngc"
        ngc_path = os.path.join(os.getcwd(), ngc_filename)
        
        with open(ngc_path, 'w') as f:
            f.write("o<canned-cycle> sub\n")
            for line in gcode_lines:
                f.write(f"{line}\n")
            f.write("o<canned-cycle> endsub\n")
        
        c.mdi("o<canned-cycle> call")
        
        return {
            "status": "OK", 
            "message": "Turning cycle started",
            "gcode": gcode_lines,
            "subroutine_file": ngc_filename
        }
        
    except Exception as e:
        error_msg = f"Error executing turning subroutine: {str(e)}"
        return {"status": "Error", "message": error_msg}, 500


@app.put("/hal/threading/generate")
def generate_threading():
    json_data = request.json
    
    if not json_data:
        return {"status": "Error", "message": "Missing threading parameters"}, 400

    try:
        gcode_lines = generate_threading_gcode_core(json_data, for_backplot=True)
        return {
            "status": "OK", 
            "message": "Threading G-code generated",
            "gcode": gcode_lines
        }
        
    except Exception as e:
        error_msg = f"Error generating threading G-code: {str(e)}"
        return {"status": "Error", "message": error_msg}, 500


@app.put("/hal/threading")
def execute_threading():
    json_data = request.json
    
    if not json_data:
        return {"status": "Error", "message": "Missing threading parameters"}, 400

    c.state(linuxcnc.STATE_ON)
    c.wait_complete()

    try:
        s = linuxcnc.stat()
        while True:
            s.poll()
            if s.estop:
                return {"status": "Error", "message": "Machine is in ESTOP state"}, 400
            if not s.enabled:
                return {"status": "Error", "message": "Machine is not enabled"}, 400
            if not s.homed:
                return {"status": "Error", "message": "Machine is not homed"}, 400
            if s.interp_state != linuxcnc.INTERP_IDLE:
                return {"status": "Error", "message": "Interpreter is not idle"}, 400
            if s.task_mode != linuxcnc.MODE_MDI:
                c.mode(linuxcnc.MODE_MDI)
                time.sleep(0.1)
                continue
            break
        c.wait_complete()

        gcode_lines = generate_threading_gcode_core(json_data, for_backplot=False)
        
        ngc_filename = "canned-cycle.ngc"
        ngc_path = os.path.join(os.getcwd(), ngc_filename)
        
        with open(ngc_path, 'w') as f:
            f.write("o<canned-cycle> sub\n")
            for line in gcode_lines:
                f.write(f"{line}\n")
            f.write("o<canned-cycle> endsub\n")
        
        c.mdi("o<canned-cycle> call")
        
        return {
            "status": "OK", 
            "message": "Canned cycle started",
            "gcode": gcode_lines,
            "subroutine_file": ngc_filename
        }
        
    except Exception as e:
        error_msg = f"Error executing threading subroutine: {str(e)}"
        return {"status": "Error", "message": error_msg}, 500


@app.put("/hal/cleanup")
def cleanup_canned_cycle_files():
    """Clean up temporary canned cycle .ngc files"""
    import os
    
    try:
        files_removed = []
        ngc_path = os.path.join(os.getcwd(), "canned-cycle.ngc")
        
        if os.path.exists(ngc_path):
            try:
                os.remove(ngc_path)
                files_removed.append("canned-cycle.ngc")
            except OSError as e:
                pass
        
        message = f"Cleaned up {len(files_removed)} canned cycle files"
        if files_removed:
            message += f": {', '.join(files_removed)}"
            
        return {
            "status": "OK",
            "message": message,
            "files_removed": files_removed
        }
        
    except Exception as e:
        error_msg = f"Error cleaning up canned cycle files: {str(e)}"
        return {"status": "Error", "message": error_msg}, 500


@app.put("/hal/hal_out")
def write_hal_out():
    json = request.json

    if "control_stop_now" in json:
        hal_pin_velocity_z_cmd.set(0)
        hal_pin_velocity_x_cmd.set(0)
        hal_pin_control_z_type.set(0)
        hal_pin_control_x_type.set(0)

    if "control_z_type" in json:
        hal_pin_control_z_type.set(json["control_z_type"])
    if "control_x_type" in json:
        hal_pin_control_x_type.set(json["control_x_type"])
    if "velocity_z_cmd" in json:
        hal_pin_velocity_z_cmd.set(json["velocity_z_cmd"])
    if "velocity_x_cmd" in json:
        hal_pin_velocity_x_cmd.set(json["velocity_x_cmd"])

    hal_pin_offset_z_encoder.set(-hal_pin_position_a.get())
    hal_pin_offset_z_stepper.set(+hal_pin_position_z.get())
    hal_pin_offset_x_encoder.set(-hal_pin_position_a.get())
    hal_pin_offset_x_stepper.set(+hal_pin_position_x.get())

    if "control_source" in json:
        hal_pin_control_source.set(json["control_source"])
    if "enable_stepper_z" in json:
        hal_pin_enable_stepper_z.set(json["enable_stepper_z"])
    if "enable_stepper_x" in json:
        hal_pin_enable_stepper_x.set(json["enable_stepper_x"])

    if "forward_z" in json:
        hal_pin_forward_z.set(json["forward_z"])
    if "enable_z" in json:
        hal_pin_enable_z.set(json["enable_z"])
    if "forward_x" in json:
        hal_pin_forward_x.set(json["forward_x"])
    if "enable_x" in json:
        hal_pin_enable_x.set(json["enable_x"])

    hal_pin_machine_is_on.set(True)

    return {"status": "OK"}


halc.ready()
haluic.ready()

print("{REST_API_READY}")

sys.stdout.flush()

if __name__ == "__main__":
    from waitress import serve

    serve(app, host="0.0.0.0", port=8000)
