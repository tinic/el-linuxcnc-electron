#!/usr/bin/env python3
import linuxcnc

from flask import Flask
from flask_cors import CORS
from flask import request

c = linuxcnc.command()
c.state(linuxcnc.STATE_ESTOP_RESET)
c.state(linuxcnc.STATE_ON)
c.wait_complete()

app = Flask(__name__)
CORS(app, resources={r"/*": {"origins": "*"}})

@app.route('/')
def index():
    return {"status": "OK!"}

@app.put("/linuxcnc/test")
def write_test():
    #json = request.json;
    c.mode(linuxcnc.MODE_MDI)
    c.wait_complete()
    c.mdi("G1 Z-10 F100")
    return {"status" : "OK!"}

if __name__ == '__main__':
    from waitress import serve
    serve(app, host='localhost', port=8001)
