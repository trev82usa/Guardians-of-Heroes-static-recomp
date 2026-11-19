#!/usr/bin/env python3
"""
Find code addresses that the game builds as constants (lis + addi/ori pairs) but that ReXGlue
did not register as functions. Calling one through a pointer (callbacks, thread entries, CRT
initialiser tables) aborts with "Call to invalid or unregistered function".

Unlike tools/find_callback_functions.py (Crash Course / Root Beer Tapper), this works only from
the generated C++ (the disassembly comments), so it needs no decoded XEX image. Pointers stored
in .data tables are not visible this way; add those from the runtime error message.

usage: find_constant_code_refs.py generated/default [existing fixes .toml ...]
Prints a [functions] TOML fragment.
"""
import glob
import os
import re
import sys

gen = sys.argv[1]
init = open(glob.glob(os.path.join(gen, '*_init.cpp'))[0]).read()
known = sorted({int(a, 16) for a in re.findall(r'\{\s*0x([0-9A-F]{8}),\s*(?:sub_|__imp__)', init)})
lo_text, hi_text = known[0], known[-1]
for f in sys.argv[2:]:
    known += [int(a, 16) for a in re.findall(r'^0x([0-9A-Fa-f]{8})\s*=', open(f).read(), re.M)]
known_set = set(known)

lis_re = re.compile(r'// lis r(\d+),(-?\d+)')
add_re = re.compile(r'// (addi|ori|addic) r(\d+),r(\d+),(-?\d+|0x[0-9a-f]+)')
func_re = re.compile(r'^DEFINE_REX_FUNC\((sub_[0-9A-F]+)\)')

found = {}
for path in sorted(glob.glob(os.path.join(gen, '*_recomp.*.cpp'))):
    regs, cur = {}, None
    for line in open(path):
        m = func_re.match(line)
        if m:
            regs, cur = {}, m.group(1)
            continue
        m = lis_re.search(line)
        if m:
            regs[int(m.group(1))] = (int(m.group(2)) & 0xFFFF) << 16
            continue
        m = add_re.search(line)
        if m:
            op, rd, ra, imm = m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4), 0)
            if ra in regs:
                v = (regs[ra] | (imm & 0xFFFF)) if op == 'ori' else (regs[ra] + imm) & 0xFFFFFFFF
                if lo_text <= v <= hi_text and v % 4 == 0 and v not in known_set:
                    found.setdefault(v, set()).add(cur)
            regs.pop(rd, None)

print('# constant-loaded code addresses not registered as functions')
print('[functions]')
for v in sorted(found):
    print(f'0x{v:08X} = {{}}  # loaded in {", ".join(sorted(found[v])[:3])}')
