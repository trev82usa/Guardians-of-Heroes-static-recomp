#!/usr/bin/env python3
"""
Find MSVC thunks that ReXGlue missed. They have no .pdata and no direct callers (only vtables
point at them), and the compiler packs them back to back:
  this-adjust thunk (8 bytes):  addi r3,r3,-N ; b target
  vcall thunk      (16 bytes):  lwz r11,0(r3) ; lwz r10,N(r11) ; mtctr r10 ; bctr
Any gap in the generated code next to a known thunk is filled with thunk-sized slots.
Works from the generated C++ only (no decoded XEX needed).

usage: find_thunk_slots.py generated/default [fixes .toml ...]   -> prints [functions] lines
"""
import glob, os, re, sys

body = {}
for p in glob.glob(os.path.join(sys.argv[1], '*_recomp.*.cpp')):
    cur = None
    for l in open(p):
        m = re.match(r'^DEFINE_REX_FUNC\(sub_([0-9A-F]+)\)', l)
        if m:
            cur = int(m.group(1), 16); body[cur] = []; continue
        if cur is not None and re.match(r'^\s+// (?!ERROR)', l):
            body[cur].append(l.strip()[3:].strip())

def thunk_size(a):
    b = body.get(a, [])
    if len(b) == 2 and re.match(r'addi r3,r3,-?\d+', b[0]) and b[1].startswith('b 0x'):
        return 8
    if len(b) == 4 and b[0] == 'lwz r11,0(r3)' and b[1].startswith('lwz r10,') and b[2] == 'mtctr r10' and b[3] == 'bctr':
        return 16
    return 0

have = set()
for f in sys.argv[2:]:
    have |= {int(a, 16) for a in re.findall(r'^0x([0-9A-Fa-f]{8})\s*=', open(f).read(), re.M)}
init = open(glob.glob(os.path.join(sys.argv[1], '*_init.cpp'))[0]).read()
have |= {int(a, 16) for a in re.findall(r'\{\s*0x([0-9A-F]{8}),', init)}
starts = sorted(body)
out = []
for a, b in zip(starts, starts[1:]):
    end = a + 4 * len(body[a])
    step = thunk_size(a) or thunk_size(b)
    if step and step <= b - end <= 0x400:
        # Thunks pack up to the next function, so align the slots to it (the gap may start
        # with alignment padding).
        out += sorted(x for x in range(b - step, end - 1, -step) if x not in have)
for x in out:
    print(f'0x{x:08X} = {{}}  # thunk slot (vtable only)')
