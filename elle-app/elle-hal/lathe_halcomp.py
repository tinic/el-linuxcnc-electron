#!/usr/bin/env python3
import hal
import sys
import linuxcnc
import time

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
        "error_state": error_state,
        "linuxcnc_actual_x": s.actual_position[0],  # X axis actual position from LinuxCNC
        "linuxcnc_actual_z": s.actual_position[2],  # Z axis actual position from LinuxCNC
        "linuxcnc_joint_x": s.joint_actual_position[0],  # Joint 0 (X) actual position
        "linuxcnc_joint_z": s.joint_actual_position[1],   # Joint 1 (Z) actual position
        "hal_raw_x": hal_pin_position_x.get(),  # Raw HAL pin for debugging
        "hal_raw_z": hal_pin_position_z.get(),   # Raw HAL pin for debugging
        "g5x_offset_x": s.g5x_offset[0],  # Work coordinate system offset X
        "g5x_offset_z": s.g5x_offset[2],  # Work coordinate system offset Z
        "g92_offset_x": s.g92_offset[0],  # G92 offset X
        "g92_offset_z": s.g92_offset[2],  # G92 offset Z
        "tool_offset_x": s.tool_offset[0],  # Tool offset X
        "tool_offset_z": s.tool_offset[2],   # Tool offset Z
        "spindle_enabled": s.spindle[0]['enabled'],  # Spindle enabled state
        "spindle_speed": s.spindle[0]['speed'],      # Spindle speed
        "spindle_direction": s.spindle[0]['direction']  # Spindle direction
    }

@app.put("/hal/abort")
def abort_operation():
    try:
        # Abort current operation without E-stop
        c.abort()
        
        print("Operation aborted")
        sys.stdout.flush()
        
        return {"status": "OK", "message": "Operation aborted"}
        
    except Exception as e:
        error_msg = f"Error during abort: {str(e)}"
        print(error_msg)
        sys.stdout.flush()
        return {"status": "Error", "message": error_msg}, 500

@app.put("/hal/estop")
def emergency_stop():
    try:
        # Immediate abort of all operations
        c.abort()
        
        # Set machine to E-stop state
        c.state(linuxcnc.STATE_ESTOP)
        
        print("Emergency stop executed")
        sys.stdout.flush()
        
        return {"status": "OK", "message": "Emergency stop executed"}
        
    except Exception as e:
        error_msg = f"Error during emergency stop: {str(e)}"
        print(error_msg)
        sys.stdout.flush()
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
                print("Error: Machine is in ESTOP state.")
                return {"status": "Error", "message": "Machine is in ESTOP state"}, 400
            if not s.enabled:
                print("Error: Machine is not enabled.")
                return {"status": "Error", "message": "Machine is not enabled"}, 400
            if not s.homed:
                print("Error: Machine is not homed.")
                return {"status": "Error", "message": "Machine is not homed"}, 400
            if s.interp_state != linuxcnc.INTERP_IDLE:
                print("Error: Interpreter is not idle.")
                return {"status": "Error", "message": "Interpreter is not idle"}, 400
            if s.task_mode != linuxcnc.MODE_MDI:
                print("Setting MDI mode")
                c.mode(linuxcnc.MODE_MDI)
                time.sleep(0.1)
                continue
            break
        c.wait_complete()

        print(s.position)

        # Set thread-loop.ngc parameters using MDI
        c.mdi("G8")       # Radius mode
        c.wait_complete()
        c.mdi("G90")      # Absolute positioning
        c.wait_complete()
        c.mdi("G21")      # Metric units (assuming parameters are in mm)
        c.wait_complete()
        c.mdi(f"G10 L20 P1 X{float(json_data['XPos']):.6f} Z{float(json_data['ZPos']):.6f}")
        c.wait_complete()
        c.mdi("G54")      # Use work coordinates
        c.wait_complete()
        c.mdi("F100")     # Set feed rate for G1 moves (100 mm/min)
        c.wait_complete()
        c.mdi("M3S500")   # Start spindle at 500 RPM
        c.wait_complete()
        
        # Set thread-loop parameters
        c.mdi(f"#<_X_Start> = {float(json_data['XStart']):.6f}")
        c.mdi(f"#<_Z_Start> = {float(json_data['ZStart']):.6f}")
        c.mdi(f"#<_Pitch> = {float(json_data['Pitch']):.6f}")
        c.mdi(f"#<_X_Depth> = {float(json_data['XDepth']):.6f}")
        c.mdi(f"#<_Z_Depth> = {float(json_data['ZDepth']):.6f}")
        c.mdi(f"#<_X_End> = {float(json_data['XEnd']):.6f}")
        c.mdi(f"#<_Z_End> = {float(json_data['ZEnd']):.6f}")
        c.mdi(f"#<_X_Pullout> = {float(json_data['XPullout']):.6f}")
        c.mdi(f"#<_Z_Pullout> = {float(json_data['ZPullout']):.6f}")
        c.mdi(f"#<_First_Cut> = {float(json_data['FirstCut']):.6f}")
        c.mdi(f"#<_Cut_Mult> = {float(json_data['CutMult']):.6f}")
        c.mdi(f"#<_Min_Cut> = {float(json_data['MinCut']):.6f}")
        c.mdi(f"#<_Spring_Cuts> = {int(json_data['SpringCuts'])}")
        c.mdi(f"#<_X_Return> = {int(json_data['XReturn'])}")
        c.mdi(f"#<_Z_Return> = {int(json_data['ZReturn'])}")
        c.wait_complete()

        # Call the thread-loop subroutine
        c.mdi("o<thread-loop> call")
        
        print(f"Executed threading subroutine with parameters: {json_data}")
        sys.stdout.flush()
        
        return {"status": "OK", "message": "Threading subroutine executed"}
        
    except Exception as e:
        error_msg = f"Error executing threading subroutine: {str(e)}"
        print(error_msg)
        sys.stdout.flush()
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
