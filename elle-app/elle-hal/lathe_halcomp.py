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
    return {
        "position_z": hal_pin_position_z.get(),
        "position_x": hal_pin_position_x.get(),
        "position_a": hal_pin_position_a.get(),
        "speed_rps": hal_pin_speed_rps.get(),
    }

@app.put("/hal/gcode")
def write_gcode():
    json_data = request.json
    
    if not json_data or "gcode" not in json_data:
        return {"status": "Error", "message": "Missing gcode parameter"}, 400
    
    gcode_command = json_data["gcode"]
    
    try:

        c.mode(linuxcnc.MODE_MDI)

        # Ensure LinuxCNC is in a state where it can accept commands
        c.wait_complete()

        s = linuxcnc.stat()
        while True:
            s.poll()
            if s.estop:
                print("Error: Machine is in ESTOP state.")
                exit()
            if not s.enabled:
                print("Error: Machine is not enabled.")
                exit()
            if not s.homed:
                print("Error: Machine is not homed.")
                exit()
            if s.interp_state != linuxcnc.INTERP_IDLE:
                print("Error: Interpreter is not idle.")
                exit()
            if s.task_mode != linuxcnc.MODE_MDI:
                print("Setting MDI mode")
                c.mode(linuxcnc.MODE_MDI)
                time.sleep(0.1)
                continue
            break

        # # Execute the G-code command
        c.mdi(gcode_command)
        
        print(f"Executed G-code: {gcode_command}")
        sys.stdout.flush()
        
        return {"status": "OK", "gcode": gcode_command}
        
    except Exception as e:
        error_msg = f"Error executing G-code '{gcode_command}': {str(e)}"
        print(error_msg)
        sys.stdout.flush()
        return {"status": "Error", "message": error_msg}, 500

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


print("{REST_API_READY}")

sys.stdout.flush()

if __name__ == "__main__":
    from waitress import serve

    serve(app, host="0.0.0.0", port=8000)
