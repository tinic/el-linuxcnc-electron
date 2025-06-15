#!/usr/bin/env python3
import os
import shutil
import json
import base64

import tempfile
import linuxcnc
import gcode
import rs274.glcanon
import rs274.interpret

from flask import Flask
from flask_cors import CORS
from flask import request

c = linuxcnc.command()

app = Flask(__name__)
CORS(app, resources={r"/*": {"origins": "*"}})


class NullProgress:
    def nextphase(self, var1):
        pass

    def progress(self):
        pass


class StatCanon(rs274.glcanon.GLCanon, rs274.interpret.StatMixin):
    def __init__(
        self, colors, geometry, is_foam, lathe_view_option, stat, random, arcdivision
    ):
        rs274.glcanon.GLCanon.__init__(self, colors, geometry)
        rs274.interpret.StatMixin.__init__(self, stat, random)
        self.progress = NullProgress()
        self.lathe_view_option = lathe_view_option
        self.arcdivision = arcdivision
        self.is_foam = is_foam

    def is_lathe(self):
        return self.lathe_view_option


def sortByLine(elem):
    return elem["line"]


class BackplotGenerator(rs274.glcanon.GlCanonDraw):
    def __init__(self, inifile):
        self.inifile = linuxcnc.ini(inifile)
        self.inifile_path = os.path.split(inifile)[0]
        self.select_primed = None
        rs274.glcanon.GlCanonDraw.__init__(self, linuxcnc.stat(), None)
        live_axis_count = 0
        for i, j in enumerate("XYZABCUVW"):
            if self.stat.axis_mask & (1 << i) == 0:
                continue
            live_axis_count += 1
        self.num_joints = int(self.inifile.find("KINS", "JOINTS") or live_axis_count)
        self.foam_option = bool(self.inifile.find("DISPLAY", "FOAM"))
        temp = self.inifile.find("DISPLAY", "LATHE")
        self.lathe_option = bool(temp == "1" or temp == "True" or temp == "true")
        self.a_axis_wrapped = bool(self.inifile.find("AXIS_A", "WRAPPED_ROTARY"))
        self.b_axis_wrapped = bool(self.inifile.find("AXIS_B", "WRAPPED_ROTARY"))
        self.c_axis_wrapped = bool(self.inifile.find("AXIS_C", "WRAPPED_ROTARY"))

    def load(self, filepath):
        self._current_file = filepath
        try:
            self.stat.poll()
            random_toolchanger = int(
                self.inifile.find("EMCIO", "RANDOM_TOOLCHANGER") or 0
            )
            arcdivision = int(self.inifile.find("DISPLAY", "ARCDIVISION") or 64)
            self.canon = StatCanon(
                None,
                self.inifile.find("DISPLAY", "GEOMETRY") or "XYZ",
                self.foam_option,
                self.lathe_option,
                self.stat,
                random_toolchanger,
                arcdivision,
            )
            parameter_file = os.path.join(
                self.inifile_path,
                os.path.basename(
                    self.inifile.find("RS274NGC", "PARAMETER_FILE") or "linuxcnc.var"
                ),
            )
            with tempfile.TemporaryDirectory() as tmpdirname:
                tmp_parameter_file = os.path.join(tmpdirname, "backplot.var")
                if os.path.exists(parameter_file):
                    shutil.copy(parameter_file, tmp_parameter_file)
                self.canon.parameter_file = tmp_parameter_file
                initcode = self.inifile.find("RS274NGC", "RS274NGC_STARTUP_CODE") or ""
                result, seq = self.load_preview(filepath, self.canon, "G18 G8 G21 G90", initcode)
                if result > gcode.MIN_ERROR:
                    pass
        finally:
            pass

    def toJson(self):
        data = []
        if self.canon.feed and self.canon.feed[0]:
            currentline = self.canon.feed[0][0]
            feed = []
            for entry in self.canon.feed:
                if currentline != entry[0]:
                    data.append({"type": "feed", "line": currentline, "feed": feed})
                    currentline = entry[0]
                    feed = []
                feed.append(
                    {
                        "coords": [
                            entry[1][0],
                            entry[1][1],
                            entry[1][2],
                            entry[2][0],
                            entry[2][1],
                            entry[2][2],
                        ],
                        "rate": entry[3],
                        "offset": [entry[4][0], entry[4][1], entry[4][2]],
                    }
                )
            data.append({"type": "feed", "line": currentline, "feed": feed})

        if self.canon.arcfeed and self.canon.arcfeed[0]:
            currentline = self.canon.arcfeed[0][0]
            arc = []
            for entry in self.canon.arcfeed:
                if entry[0] != currentline:
                    data.append(
                        {"type": "arcfeed", "line": currentline, "arcfeed": arc}
                    )
                    currentline = entry[0]
                    arc = []
                arc.append(
                    {
                        "coords": [
                            entry[1][0],
                            entry[1][1],
                            entry[1][2],
                            entry[2][0],
                            entry[2][1],
                            entry[2][2],
                        ],
                        "rate": entry[3],
                        "offset": [entry[4][0], entry[4][1], entry[4][2]],
                    }
                )
            data.append({"type": "arcfeed", "line": currentline, "arcfeed": arc})

        if self.canon.traverse and self.canon.traverse[0]:
            currentline = self.canon.traverse[0][0]
            trav = []
            for entry in self.canon.traverse:
                if entry[0] != currentline:
                    data.append({"type": "trav", "line": currentline, "trav": trav})
                    trav = []
                    currentline = entry[0]
                trav.append(
                    {
                        "coords": [
                            entry[1][0],
                            entry[1][1],
                            entry[1][2],
                            entry[2][0],
                            entry[2][1],
                            entry[2][2],
                        ],
                        "offset": [entry[3][0], entry[3][1], entry[3][2]],
                    }
                )
            data.append({"type": "trav", "line": currentline, "trav": trav})

        if self.canon.dwells and self.canon.dwells[0]:
            currentline = self.canon.dwells[0][0]
            trav = []
            for entry in self.canon.dwells:
                if entry[0] != currentline:
                    data.append({"type": "dwell", "line": currentline, "trav": trav})
                    trav = []
                    currentline = entry[0]
                trav.append(
                    {"color": [entry[1]], "coord": [entry[0], entry[1], entry[2]]}
                )
            data.append({"type": "dwell", "line": currentline, "dwell": trav})

        data.sort(key=sortByLine)
        
        min_x = min_y = min_z = float('inf')
        max_x = max_y = max_z = float('-inf')
        
        for entry in data:
            entry_type = entry["type"]
            if entry_type in ["feed", "arcfeed", "trav"]:
                moves = entry.get(entry_type, [])
                for move in moves:
                    coords = move.get("coords", [])
                    if len(coords) >= 6:
                        min_x = min(min_x, coords[0])
                        min_y = min(min_y, coords[1])
                        min_z = min(min_z, coords[2])
                        max_x = max(max_x, coords[0])
                        max_y = max(max_y, coords[1])
                        max_z = max(max_z, coords[2])
                        
                        min_x = min(min_x, coords[3])
                        min_y = min(min_y, coords[4])
                        min_z = min(min_z, coords[5])
                        max_x = max(max_x, coords[3])
                        max_y = max(max_y, coords[4])
                        max_z = max(max_z, coords[5])
        
        if min_x == float('inf'):
            min_x = min_y = min_z = -1.0
            max_x = max_y = max_z = 1.0
        
        # Store original extents before transformation
        original_min_x, original_min_y, original_min_z = min_x, min_y, min_z
        original_max_x, original_max_y, original_max_z = max_x, max_y, max_z
        
        # Check if LinuxCNC interpreted coordinates as inches instead of mm
        # If the coordinate range is much smaller than expected, apply conversion
        coordinate_range = max(abs(max_x - min_x), abs(max_y - min_y), abs(max_z - min_z))
        if coordinate_range > 0 and coordinate_range < 10:  # Suspiciously small for typical machining
            # Apply inch to mm conversion (25.4x) to all coordinates
            units_scale = 25.4
            min_x *= units_scale
            min_y *= units_scale 
            min_z *= units_scale
            max_x *= units_scale
            max_y *= units_scale
            max_z *= units_scale
            
            original_min_x *= units_scale
            original_min_y *= units_scale
            original_min_z *= units_scale
            original_max_x *= units_scale
            original_max_y *= units_scale
            original_max_z *= units_scale
            
            # Scale all coordinate data
            for entry in data:
                entry_type = entry["type"]
                if entry_type in ["feed", "arcfeed", "trav"]:
                    moves = entry.get(entry_type, [])
                    for move in moves:
                        coords = move.get("coords", [])
                        if len(coords) >= 6:
                            for i in range(6):
                                coords[i] *= units_scale
        
        center_x = (min_x + max_x) / 2
        center_y = (min_y + max_y) / 2
        center_z = (min_z + max_z) / 2
        
        range_x = abs(max_x - min_x)
        range_y = abs(max_y - min_y) 
        range_z = abs(max_z - min_z)
        max_range = max(range_x, range_y, range_z)
        
        scale_factor = 2.0 / max_range if max_range > 0 else 1.0
        
        for entry in data:
            entry_type = entry["type"]
            if entry_type in ["feed", "arcfeed", "trav"]:
                moves = entry.get(entry_type, [])
                for move in moves:
                    coords = move.get("coords", [])
                    if len(coords) >= 6:
                        x1 = (coords[0] - center_x) * scale_factor
                        y1 = (coords[1] - center_y) * scale_factor
                        z1 = (coords[2] - center_z) * scale_factor
                        
                        x2 = (coords[3] - center_x) * scale_factor
                        y2 = (coords[4] - center_y) * scale_factor
                        z2 = (coords[5] - center_z) * scale_factor
                        
                        coords[0] = z1
                        coords[1] = y1
                        coords[2] = x1
                        
                        coords[3] = z2
                        coords[4] = y2
                        coords[5] = x2
        
        norm_min_x = (min_x - center_x) * scale_factor
        norm_min_y = (min_y - center_y) * scale_factor
        norm_min_z = (min_z - center_z) * scale_factor
        norm_max_x = (max_x - center_x) * scale_factor
        norm_max_y = (max_y - center_y) * scale_factor
        norm_max_z = (max_z - center_z) * scale_factor
        
        min_x = norm_min_z
        min_y = norm_min_y
        min_z = norm_min_x
        max_x = norm_max_z
        max_y = norm_max_y
        max_z = norm_max_x
        
        
        rootData = {
            "backplot": data,
            "extents": [min_x, min_y, min_z, max_x, max_y, max_z],
            "transform": {
                "center": [center_x, center_y, center_z],
                "scale_factor": scale_factor,
                "original_min": [original_min_x, original_min_y, original_min_z],
                "original_max": [original_max_x, original_max_y, original_max_z]
            }
        }
        return json.dumps(rootData, separators=(",", ":"))


@app.route("/")
def index():
    return {"status": "OK"}


@app.put("/linuxcnc/backplot")
def backplot():
    with tempfile.TemporaryDirectory() as tmpdirname:
        file_path = os.path.join(tmpdirname, "gcode.ngc")
        file = open(file_path, "w")
        gcode_bytes = base64.b64decode(request.json["gcode"])
        gcode_string = gcode_bytes.decode("ascii")
        file.write(gcode_string)
        file.close()
        lathe_init_path = os.path.join(os.getcwd(), "lathe.ini")
        bp = BackplotGenerator(lathe_init_path)
        bp.load(file_path)
        
        result = bp.toJson()
        return result


if __name__ == "__main__":
    from waitress import serve

    serve(app, host="0.0.0.0", port=8001)
