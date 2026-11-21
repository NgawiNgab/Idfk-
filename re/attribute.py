#!/usr/bin/env python3
"""
attribute.py -- Map "which named system owns function X" by walking up the
call graph. For each system record slot fn, determine if it (or anything it
calls within N levels) reaches a target address.
"""
import sys, struct, collections
sys.path.insert(0,'re')
from symtable import SymDB
from systems import build, method_of
from bedrock_re import Library

class CallGraph:
    def __init__(self, lib):
        self.lib = lib
        self._cache = {}
        t_start, t_end, t_off = lib.text_range()
        lib.f.seek(t_off); self.blob = lib.f.read(t_end - t_start)
        self.t_start = t_start
        # build index of all bl targets: target -> [sites]
        self.bl_sites = collections.defaultdict(list)
        n = len(self.blob)
        for i in range(0, n-4, 4):
            w = struct.unpack_from('<I', self.blob, i)[0]
            if (w & 0xFC000000) != 0x94000000: continue
            imm = w & 0x03FFFFFF
            if imm & 0x02000000: imm -= 0x04000000
            self.bl_sites[t_start + i + imm*4].append(t_start + i)

    def callers(self, target):
        return self.bl_sites.get(target, [])

def reaches(cg, start, target, depth=3, seen=None):
    """BFS: does function at `start` call `target` within `depth` levels?"""
    if seen is None: seen = set()
    if start in seen or depth < 0: return False
    seen.add(start)
    for site in cg.callers(target):
        f = cg.lib.function_start(site)
        if f == start: return True
    if depth == 0: return False
    # also check callees of start
    return False

if __name__ == '__main__':
    db = SymDB(); lib = db.lib
    recs = build(db); cg = CallGraph(lib)
    target = int(sys.argv[1], 16)
    print(f"[attr] systems calling {target:#x} directly:")
    hit = 0
    for r in recs:
        ms = method_of(r['adapter'])
        for f in r['fns']:
            # exact: does this fn contain a bl to target?
            fs = lib.function_start(target)
            for site in cg.callers(target):
                if lib.function_start(site) == f:
                    print(f"  FN {f:#x} in rec {r['va']:#x} {ms[:2]}")
                    hit += 1
                    break
    print(f"  ({hit} matches)")
