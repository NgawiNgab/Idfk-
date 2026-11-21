#!/usr/bin/env python3
"""
nameres.py -- Layout-agnostic resolution of named Bedrock functions.

Bedrock's .data.rel.ro holds many system-metadata structs with varying layouts,
but every one of them stores the system's mangled name as an R_AARCH64_RELATIVE
pointer. So:
    name string --(reloc)--> pointer slot --(neighbouring reloc)--> fns
For each query we report the function pointers in the same struct, and flag the
ones whose body differs from the shared boilerplate (those are the real impls).
"""
import sys, re, collections
sys.path.insert(0,'re')
from symtable import SymDB
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

BOILER = {0x8a7ac94, 0x1346b978}

def _read_len_name(s, i):
    """Read <decimal-len><that many chars> starting at s[i]. Returns (name, next_i)."""
    j = i
    while j < len(s) and s[j].isdigit(): j += 1
    if j == i: return None, i
    n = int(s[i:j])
    return s[j:j+n], j+n

def method_pairs(s):
    """Extract 'Class::method' from a mangled Bedrock name (length-prefixed)."""
    out = []
    for m in re.finditer(r'_ZN', s):
        i = m.end()
        parts = []
        while i < len(s) and s[i] != 'E' and len(parts) < 2:
            nm, i2 = _read_len_name(s, i)
            if nm is None: break
            parts.append(nm.split('.')[-1]); i = i2
        if len(parts) == 2 and parts[0][:1].isupper():
            out.append(f"{parts[0]}::{parts[1]}")
    if out: return out
    for m in re.finditer(r'(\d+)([A-Z][A-Za-z0-9_]{3,40})(\d+)([a-z]\w{2,40})', s):
        out.append(f"{m.group(2)}::{m.group(4)}")
    return out

class Resolver:
    def __init__(self, db):
        self.db = db; self.lib = db.lib
        self.slot2name = collections.defaultdict(list)      # slot -> [names]
        self.name2slots = collections.defaultdict(list)
        for w, a in db.relocs:
            if db.ro_start <= a < db.ro_end:
                s = self.lib.read(a, 500).split(b'\0')[0].decode('latin1', 'replace')
                if '_ZN' in s or 'System' in s:
                    self.slot2name[w].append(s)
                    for m in method_pairs(s):
                        self.name2slots[m].append((w, a, s))

    def funcs_for(self, method, span=0x90, limit=12):
        """Return function pointers living in the same struct as `method`."""
        out = []
        for slot, nva, s in self.name2slots.get(method, []):
            near = [(w - slot, w, a) for w, a in self.db.relocs
                    if abs(w - slot) <= span and self.db.t_start <= a < self.db.t_end]
            near.sort()
            out.append(dict(slot=slot, name_va=nva, raw=s,
                            fns=[(d, w, a) for d, w, a in near]))
        return out

    def all_methods(self):
        return sorted(self.name2slots)

if __name__ == '__main__':
    db = SymDB(); R = Resolver(db); lib = db.lib
    qs = sys.argv[1:]
    if not qs:
        ms = R.all_methods()
        print(f"[nameres] {len(ms)} resolvable methods")
        for m in ms: print("   ", m)
        sys.exit(0)
    for q in qs:
        cand = [m for m in R.all_methods() if q in m]
        print(f"\n########## /{q}/  {len(cand)} matches")
        for m in cand[:6]:
            print(f"\n=== {m}")
            for info in R.funcs_for(m)[:2]:
                print(f"   slot {info['slot']:#x}")
                for d, w, a in info['fns']:
                    tag = 'boiler' if a in BOILER else 'IMPL  '
                    print(f"     {d:+04x} {w:#x}  {tag} {a:#x}")
