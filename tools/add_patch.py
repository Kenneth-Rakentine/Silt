#!/usr/bin/env python3
"""Adds a patch copied from the app (Copy patch) to lab/patches.js as a factory patch.
Usage: python3 tools/add_patch.py patch.json "Factory name" "target" "listen" "decides" "play"
Then run: node tools/gen_patches.js"""
import json, sys, os
path, name, target, listen, decides, play = sys.argv[1:7]
p = json.load(open(path))
P = {k: (round(v, 4) if isinstance(v, float) else v) for k, v in p['P'].items() if k not in ('listen', 'tap', 'abTrim')}
if 'bands' in P: P['bands'] = [round(b, 3) for b in P['bands']]
S = [{k: round(v, 4) for k, v in s.items()} for s in p['S']]
notes = {'target': target, 'listen': listen, 'decides': decides, 'play': play}
f = os.path.join(os.path.dirname(__file__), '..', 'lab', 'patches.js')
src = open(f).read()
entry = "\n  " + json.dumps(name) + ": {\n    P: " + json.dumps(P) + ",\n    S: " + json.dumps(S) + ",\n    notes: " + json.dumps(notes) + " },\n"
i = src.index('};\nfunction makePatch')
open(f, 'w').write(src[:i] + entry + src[i:])
print('added', name)
