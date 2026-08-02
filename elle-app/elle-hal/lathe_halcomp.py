#!/usr/bin/env python3
import hal
import sys
import linuxcnc
import time
import os

from flask import Flask
from flask_cors import CORS
from flask import request

from gcode_gen import generate_threading_gcode, generate_turning_gcode, wrap_subroutine
from api_models import (
    ApiValidationError,
    HalIn,
    HalOut,
    ThreadingExecuteParams,
    ThreadingParams,
    TurningExecuteParams,
    TurningParams,
    dump,
    validate,
)

halc = hal.component("lathe")
haluic = hal.component("halui")
c = linuxcnc.command()
reset_z = 0
reset_x = 0

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

hal_pin_position_z_encoder = halc.newpin("position_z_encoder", hal.HAL_FLOAT, hal.HAL_IN)
hal_pin_position_x_encoder = halc.newpin("position_x_encoder", hal.HAL_FLOAT, hal.HAL_IN)
hal_pin_offset_z_encoder = halc.newpin("offset_z_encoder", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_offset_z_stepper = halc.newpin("offset_z_stepper", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_offset_x_encoder = halc.newpin("offset_x_encoder", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_offset_x_stepper = halc.newpin("offset_x_stepper", hal.HAL_FLOAT, hal.HAL_OUT)

hal_pin_control_z_type = halc.newpin("control_z_type", hal.HAL_BIT, hal.HAL_OUT)
hal_pin_control_x_type = halc.newpin("control_x_type", hal.HAL_BIT, hal.HAL_OUT)
hal_pin_velocity_z_cmd = halc.newpin("velocity_z_cmd", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_velocity_x_cmd = halc.newpin("velocity_x_cmd", hal.HAL_FLOAT, hal.HAL_OUT)

hal_pin_reset_z = halc.newpin("reset_z", hal.HAL_U32, hal.HAL_OUT)
hal_pin_reset_x = halc.newpin("reset_x", hal.HAL_U32, hal.HAL_OUT)

hal_pin_scale_encoder_z = halc.newpin("scale_encoder_z", hal.HAL_FLOAT, hal.HAL_OUT)
hal_pin_scale_encoder_x = halc.newpin("scale_encoder_x", hal.HAL_FLOAT, hal.HAL_OUT)

app = Flask(__name__)
CORS(app, resources={r"/*": {"origins": "*"}})

@app.errorhandler(ApiValidationError)
def handle_validation_error(e):
    return {"status": "Error", "message": str(e)}, 400


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
    
    return dump(HalIn(
        position_z=hal_pin_position_z.get(),
        position_x=hal_pin_position_x.get(),
        position_a=hal_pin_position_a.get(),
        speed_rps=hal_pin_speed_rps.get(),
        program_running=program_running,
        error_state=error_state,
    ))

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


@app.put("/hal/turning/generate")
def generate_turning():
    json_data = dump(validate(TurningParams, request.json))

    try:
        gcode_lines = generate_turning_gcode(json_data, for_backplot=True)
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
    json_data = dump(validate(TurningExecuteParams, request.json))

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

        gcode_lines = generate_turning_gcode(json_data, for_backplot=False)
        
        ngc_filename = "canned-cycle.ngc"
        ngc_path = os.path.join(os.getcwd(), ngc_filename)
        
        with open(ngc_path, 'w') as f:
            f.write(wrap_subroutine(gcode_lines))
        
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
    json_data = dump(validate(ThreadingParams, request.json))

    try:
        gcode_lines = generate_threading_gcode(json_data, for_backplot=True)
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
    json_data = dump(validate(ThreadingExecuteParams, request.json))

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

        gcode_lines = generate_threading_gcode(json_data, for_backplot=False)
        
        ngc_filename = "canned-cycle.ngc"
        ngc_path = os.path.join(os.getcwd(), ngc_filename)
        
        with open(ngc_path, 'w') as f:
            f.write(wrap_subroutine(gcode_lines))
        
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
    global reset_z, reset_x
    json = dump(validate(HalOut, request.json), exclude_unset=True)

    if "control_stop_now" in json:
        hal_pin_velocity_z_cmd.set(0)
        hal_pin_velocity_x_cmd.set(0)
        hal_pin_control_z_type.set(0)
        hal_pin_control_x_type.set(0)

    if "reset_position" in json:
        reset_z = reset_z + 1
        hal_pin_reset_z.set(reset_z)
        reset_x = reset_x + 1
        hal_pin_reset_x.set(reset_x)
        c.mode(linuxcnc.MODE_MDI)
        c.wait_complete()
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
        c.state(linuxcnc.STATE_OFF)
        c.wait_complete()
        c.state(linuxcnc.STATE_ON)
        c.wait_complete()
        c.reset_interpreter()
        c.wait_complete()

    # Set encoder scale factors from frontend settings or use defaults
    hal_pin_scale_encoder_z.set(json.get("encoder_scale_z", 0.001))
    hal_pin_scale_encoder_x.set(json.get("encoder_scale_x", -0.001))

    hal_pin_offset_z_encoder.set(-hal_pin_position_a.get())
    hal_pin_offset_z_stepper.set(+hal_pin_position_z_encoder.get())
    hal_pin_offset_x_encoder.set(-hal_pin_position_a.get())
    hal_pin_offset_x_stepper.set(+hal_pin_position_x_encoder.get())

    if "control_z_type" in json:
        hal_pin_control_z_type.set(json["control_z_type"])
    if "control_x_type" in json:
        hal_pin_control_x_type.set(json["control_x_type"])
    if "velocity_z_cmd" in json:
        hal_pin_velocity_z_cmd.set(json["velocity_z_cmd"])
    if "velocity_x_cmd" in json:
        hal_pin_velocity_x_cmd.set(json["velocity_x_cmd"])

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

hal_pin_reset_z.set(reset_z)
hal_pin_reset_x.set(reset_x)

time.sleep(0.500)

reset_z = reset_z + 1
hal_pin_reset_z.set(reset_z)
reset_x = reset_x + 1
hal_pin_reset_x.set(reset_x)

print("{REST_API_READY}")

sys.stdout.flush()

if __name__ == "__main__":
    from waitress import serve

    serve(app, host="0.0.0.0", port=8000)
