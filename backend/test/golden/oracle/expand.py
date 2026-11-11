"""Feed derived params through the REAL lathe_halcomp.py generators (AST-extracted, no hal/linuxcnc import).
Usage: python3 -I expand.py <lathe_halcomp.py> <derived.json> <out.json>"""
import ast, contextlib, io, json, sys

src = open(sys.argv[1]).read()
tree = ast.parse(src)
wanted = {"generate_threading_gcode_core", "generate_turning_gcode_core"}
mod = ast.Module(body=[n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in wanted], type_ignores=[])
ns: dict = {}
exec(compile(mod, sys.argv[1], "exec"), ns)

cases = json.load(open(sys.argv[2]))
for c in cases:
    fn = ns["generate_threading_gcode_core" if c["kind"] == "threading" else "generate_turning_gcode_core"]
    for variant, backplot in (("execute", False), ("backplot", True)):
        with contextlib.redirect_stdout(io.StringIO()):  # swallow the generators' debug prints
            c[variant] = fn(dict(c["params"]), for_backplot=backplot)
json.dump(cases, open(sys.argv[3], "w"), indent=1, ensure_ascii=False)
print(len(cases), "cases")
