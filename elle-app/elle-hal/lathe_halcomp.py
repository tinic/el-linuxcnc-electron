#!/usr/bin/env python3
import hal
import sys

from flask import Flask
from flask_cors import CORS
from flask import request

halc = hal.component("lathe")

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

@app.route('/')
def index():
    return {"status": "OK!"}

@app.get("/hal/hal_in")
def read_hal_in():
    return {"position_z": hal_pin_position_z.get(),
            "position_x": hal_pin_position_x.get(),
            "position_a": hal_pin_position_a.get(),
            "speed_rps": hal_pin_speed_rps.get()}

@app.put("/hal/hal_out")
def write_hal_out():
    json = request.json;

    hal_pin_offset_z_encoder.set(-hal_pin_position_a.get())
    hal_pin_offset_z_stepper.set(+hal_pin_position_z.get())
    hal_pin_offset_x_encoder.set(-hal_pin_position_a.get())
    hal_pin_offset_x_stepper.set(+hal_pin_position_x.get())

    if ("control_source" in json):
        hal_pin_control_source.set(json["control_source"])
    if ("enable_stepper_z" in json):
        hal_pin_enable_stepper_z.set(json["enable_stepper_z"])
    if ("enable_stepper_x" in json):
        hal_pin_enable_stepper_x.set(json["enable_stepper_x"])

    if ("forward_z" in json):
        hal_pin_forward_z.set(json["forward_z"])
    if ("enable_z" in json):
        hal_pin_enable_z.set(json["enable_z"])
    if ("forward_x" in json):
        hal_pin_forward_x.set(json["forward_x"])
    if ("enable_x" in json):
        hal_pin_enable_x.set(json["enable_x"])

    if ("control_z_type" in json):
        hal_pin_control_z_type.set(json["control_z_type"])
    if ("control_x_type" in json):
        hal_pin_control_x_type.set(json["control_x_type"])
    if ("velocity_z_cmd" in json):
        hal_pin_velocity_z_cmd.set(json["velocity_z_cmd"])
    if ("velocity_x_cmd" in json):
        hal_pin_velocity_x_cmd.set(json["velocity_x_cmd"])

    return {"status" : "OK!"}

halc.ready()

print("{REST_API_READY}")

sys.stdout.flush()

if __name__ == '__main__':
    from waitress import serve
    serve(app, host='0.0.0.0', port=8000)
