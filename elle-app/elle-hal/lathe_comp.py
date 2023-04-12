#!/usr/bin/env python3
import hal
import sys

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi import Request

halc = hal.component("lathe")

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

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_credentials=True,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
async def root():
    return {"status": "OK!"}

@app.get("/hal/hal_in")
async def read_hal_in():
    return {"position_z": hal_pin_position_z.get(),
            "position_x": hal_pin_position_x.get(),
            "position_a": hal_pin_position_a.get(),
            "speed_rps": hal_pin_speed_rps.get()}

@app.put("/hal/hal_out")
async def write_hal_out(request: Request):
    jsonData = await request.json();

    if ("enable_stepper_z" in jsonData):
        hal_pin_enable_stepper_z.set(jsonData["enable_stepper_z"])
    if ("enable_stepper_x" in jsonData):
        hal_pin_enable_stepper_x.set(jsonData["enable_stepper_x"])

    hal_pin_offset_z_encoder.set(-hal_pin_position_a.get())
    hal_pin_offset_z_stepper.set(+hal_pin_position_z.get())
    hal_pin_offset_x_encoder.set(-hal_pin_position_a.get())
    hal_pin_offset_x_stepper.set(+hal_pin_position_x.get())

    if ("forward_z" in jsonData):
        hal_pin_forward_z.set(jsonData["forward_z"])
    if ("enable_z" in jsonData):
        hal_pin_enable_z.set(jsonData["enable_z"])
    if ("forward_x" in jsonData):
        hal_pin_forward_x.set(jsonData["forward_x"])
    if ("enable_x" in jsonData):
        hal_pin_enable_x.set(jsonData["enable_x"])

    if ("control_z_type" in jsonData):
        hal_pin_control_z_type.set(jsonData["control_z_type"])
    if ("control_x_type" in jsonData):
        hal_pin_control_x_type.set(jsonData["control_x_type"])
    if ("velocity_z_cmd" in jsonData):
        hal_pin_velocity_z_cmd.set(jsonData["velocity_z_cmd"])
    if ("velocity_x_cmd" in jsonData):
        hal_pin_velocity_x_cmd.set(jsonData["velocity_x_cmd"])

halc.ready()

print("Python REST service ready!")

sys.stdout.flush()
