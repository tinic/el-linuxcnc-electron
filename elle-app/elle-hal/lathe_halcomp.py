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

@app.put("/hal/threading_original")
def execute_threading_original():
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


        # Build G-code array for threading operation
        gcode_lines = []
        
        # Set thread-loop.ngc parameters using MDI
        gcode_lines.append("G8")  # Radius mode
        gcode_lines.append("G90")  # Absolute positioning
        gcode_lines.append("G21")  # Metric units (assuming parameters are in mm)
        gcode_lines.append(f"G10 L20 P1 X{float(json_data['XPos']):.6f} Z{float(json_data['ZPos']):.6f}")
        gcode_lines.append("G54")  # Use work coordinates
        gcode_lines.append("F100")  # Set feed rate for G1 moves (100 mm/min)
        gcode_lines.append("M3S500")  # Start spindle at 500 RPM
        
        # Set thread-loop parameters
        gcode_lines.append(f"#<_X_Start> = {float(json_data['XStart']):.6f}")
        gcode_lines.append(f"#<_Z_Start> = {float(json_data['ZStart']):.6f}")
        gcode_lines.append(f"#<_Pitch> = {float(json_data['Pitch']):.6f}")
        gcode_lines.append(f"#<_X_Depth> = {float(json_data['XDepth']):.6f}")
        gcode_lines.append(f"#<_Z_Depth> = {float(json_data['ZDepth']):.6f}")
        gcode_lines.append(f"#<_X_End> = {float(json_data['XEnd']):.6f}")
        gcode_lines.append(f"#<_Z_End> = {float(json_data['ZEnd']):.6f}")
        gcode_lines.append(f"#<_X_Pullout> = {float(json_data['XPullout']):.6f}")
        gcode_lines.append(f"#<_Z_Pullout> = {float(json_data['ZPullout']):.6f}")
        gcode_lines.append(f"#<_First_Cut> = {float(json_data['FirstCut']):.6f}")
        gcode_lines.append(f"#<_Cut_Mult> = {float(json_data['CutMult']):.6f}")
        gcode_lines.append(f"#<_Min_Cut> = {float(json_data['MinCut']):.6f}")
        gcode_lines.append(f"#<_Spring_Cuts> = {int(json_data['SpringCuts'])}")
        gcode_lines.append(f"#<_X_Return> = {int(json_data['XReturn'])}")
        gcode_lines.append(f"#<_Z_Return> = {int(json_data['ZReturn'])}")
        
        # Call the thread-loop subroutine
        gcode_lines.append("o<thread-loop> call")
        
        # Execute each G-code line via MDI
        for gcode_line in gcode_lines:
            c.mdi(gcode_line)
            c.wait_complete()
        
        
        return {
            "status": "OK", 
            "message": "Threading subroutine executed",
            "gcode": gcode_lines
        }
        
    except Exception as e:
        error_msg = f"Error executing threading subroutine: {str(e)}"
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
    
    # Calculate compound distance and direction ratios (from thread-loop.ngc lines 32-35)
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
