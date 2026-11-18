import json, sys
p1 = "3ds/tools/azt.d/NREC4.json"
p2 = "3ds/tools/bench.d/NREC4.json"
FX = "fxtest=50,55,63,81,84,105,124"
scen = {
  "name": "fxp_snapchk", "status": "unverified",
  "subject": "NREC4 particle snaps: the 7 nd_parts_add effects (shimmer_burst 50, effect_3D 55, snowfall 63, snaking_static 81, water_block 84, embers 105, pink_sparkles 124) on mac_01 (pos 0 0 200), cold det start with the snapchk switch (no drec), no shots.",
  "proves": "NREC4: the instance snap table of the particle runs gives the lsnap counters (lists, snaps, tmis) and the snapchk bad count of the parts draws.",
  "debug": "warp=mac_01:0 pos=0:0:200 " + FX + " loglevel=debug det snapchk",
  "until": "lsnap f800 ", "cap": 30, "deadline": 25,
  "first_marker": "warp: mac_01 entry 0", "expects": "warp", "needs_picture": False,
  "expected_events": [ {"name": "warp", "regex": "warp: mac_01 entry 0"}, {"name": "lsnap", "regex": "lsnap f800 "} ],
  "reason": "NREC4: copy of fxp_drec with the snapchk switch in place of drec. Cap 30 s, deadline 25 s, until the lsnap line of frame 800."
}
drec = {
  "name": "fxparts_drec", "status": "unverified",
  "subject": "NREC4 parts effects: the 7 nd_parts_add effects (shimmer_burst 50, effect_3D 55, snowfall 63, snaking_static 81, water_block 84, embers 105, pink_sparkles 124) on mac_01 (pos 0 0 200), cold det start with the drec switch (frames 412 to 676, step 44), no shots.",
  "proves": "NREC4: the draw record of the nd_parts_add effects is byte-equal between a base and a conversion head.",
  "debug": "warp=mac_01:0 pos=0:0:200 " + FX + " loglevel=debug det drec=412:7:44",
  "until": "drec f676 draws [1-9]\d* bytes \d+", "cap": 25, "deadline": 20,
  "first_marker": "warp: mac_01 entry 0", "expects": "warp", "needs_picture": False,
  "expected_events": [ {"name": "warp", "regex": "warp: mac_01 entry 0"}, {"name": "drec", "regex": "drec f676 draws [1-9]\d* bytes \d+"} ],
  "reason": "NREC4: copy of fxp_drec with the parts effect list. Cap 25 s, deadline 20 s, until the drec line of frame 676."
}
b_snap = {
  "name": "fxp_snapchk", "kind": "azahar_stream", "speed": "normal", "default": False,
  "purpose": "NREC4: lsnap and snapchk line counters of the nd_parts_add effects (det cold start).",
  "scenario": "fxp_snapchk", "cap": 30, "until": "lsnap f800 ",
  "tags": ["azahar", "det", "snapchk"], "gates": []
}
b_drec = {
  "name": "fxparts_drec", "kind": "azahar_pic", "speed": "normal", "default": False,
  "purpose": "NREC4: drec.txt of the nd_parts_add effects, frames 412 to 676 step 44 (cold det start). Two runs of one build byte-equal.",
  "scenario": "fxparts_drec", "cap": 25, "until": "drec f676 draws [1-9]\d* bytes \d+",
  "tags": ["azahar", "det", "cold", "drec"], "gates": []
}
def dump(path, obj):
    s = json.dumps(obj, indent=2).replace("\n", "\r\n") + "\r\n"
    open(path, "w", newline="").write(s)
dump(p1, {"scenarios": [scen, drec]})
dump(p2, {"benches": [b_snap, b_drec]})
print("ok")
