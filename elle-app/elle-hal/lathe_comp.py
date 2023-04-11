#!/usr/bin/env python3
import sys
import os
import linuxcnc
import hal
import math
import time
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

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
halc.ready()

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

@app.put("/hal/hal_out/forward_z")
async def write_forward_z(item:str, value:float):
    if (item == "value"):
        print("write_forward_z {%f}",format(value));
        hal_pin_forward_z.set(value)

@app.put("/hal/hal_out/forward_x")
async def write_forward_x(item:str, value:float):
    if (item == "value"):
        print("write_forward_x {%f}",format(value));
        hal_pin_forward_x.set(value)

@app.put("/hal/hal_out/enable_z")
async def write_enable_z(item:str, value:float):
    if (item == "value"):
        hal_pin_enable_z.set(value)

@app.put("/hal/hal_out/enable_x")
async def write_enable_x(item:str, value:float):
    if (item == "value"):
        hal_pin_enable_x.set(value)

@app.put("/hal/hal_out/enable_stepper_x")
async def write_enable_x(item:str, value:bool):
    if (item == "value"):
        hal_pin_enable_stepper_x.set(value)

@app.put("/hal/hal_out/enable_stepper_z")
async def write_enable_z(item:str, value:bool):
    if (item == "value"):
        hal_pin_enable_stepper_z.set(value)

print("Python REST service ready!")
