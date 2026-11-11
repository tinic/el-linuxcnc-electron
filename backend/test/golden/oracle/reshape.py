"""Reshape golden.json into the typed layout the C++ tests read. Usage: python3 -I reshape.py golden.json cycles.json"""
import json, sys
T = {"XPos":"x_pos","ZPos":"z_pos","APos":"a_pos","XStart":"x_start","ZStart":"z_start","Pitch":"pitch","XDepth":"x_depth",
     "ZDepth":"z_depth","XEnd":"x_end","ZEnd":"z_end","XReturn":"x_return","ZReturn":"z_return","XPullout":"x_pullout",
     "ZPullout":"z_pullout","FirstCut":"first_cut","CutMult":"cut_mult","MinCut":"min_cut","SpringCuts":"spring_cuts",
     "Stock":"stock","Target":"target","ZLead":"z_lead","Angle":"angle_deg","StepDown":"step_down",
     "FinalStepDown":"final_step_down","SpringPasses":"spring_passes"}
out = {"threading": [], "turning": []}
for c in json.load(open(sys.argv[1])):
    out[c["kind"]].append({"name": c["name"], "position": c["position"], "request": c["user"],
                           "program": {T[k]: v for k, v in c["params"].items()},
                           "machine": c["execute"], "preview": c["backplot"]})
json.dump(out, open(sys.argv[2], "w"), indent=1, ensure_ascii=False)
print({k: len(v) for k, v in out.items()})
