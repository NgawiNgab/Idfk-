#!/usr/bin/env python3
"""
symtable.py -- Resolve Bedrock ECS system metadata tables.

Bedrock registers entt systems as static metadata structs in .data.rel.ro:
    { const char* name; uint64 nameLen; void(*tickFn)(...); ... }
The .so keeps R_AARCH64_RELATIVE relocs, so each pointer slot's addend is the
final VA. Find slots whose addend == name-string VA -> sibling slot is the fn.
"""
import sys, struct, collections
sys.path.insert(0, 're')
from bedrock_re import Library
from elftools.elf.elffile import ELFFile

class SymDB:
    def __init__(self, path='mods/apollon/game/libminecraftpe.so'):
        self.path = path
        self.lib = Library(path)
        self.elf = ELFFile(open(path, 'rb'))
        self.t_start, self.t_end, self.t_off = self.lib.text_range()
        self.ro_start, self.ro_end, self.ro_off = self.lib.rodata_range()
        self.relocs = []            # (where_va, addend)
        self._load_relocs()

    def _load_relocs(self):
        for sec in self.elf.iter_sections():
            if sec.header.sh_type not in ('SHT_RELA', 'SHT_REL'): continue
            for r in sec.iter_relocations():
                t = r['r_info_type']
                if t == 1027:                       # R_AARCH64_RELATIVE
                    self.relocs.append((r['r_offset'], r['r_addend']))
        print(f"[db] {len(self.relocs)} R_AARCH64_RELATIVE relocations")

    def strings(self, minlen=8):
        blob = self.lib.read(self.ro_start, self.ro_end - self.ro_start)
        out = []; i=0; n=len(blob); rs=None
        while i <= n:
            c = blob[i] if i < n else 0
            if 32 <= c < 127:
                if rs is None: rs = i
            else:
                if rs is not None and i-rs >= minlen:
                    out.append((self.ro_start+rs, blob[rs:i].decode('latin1')))
                rs=None
            i+=1
        return out

    def resolve(self, sub, verbose=True):
        """Return list of (name_va, name, table_va, fn_va) for strings containing sub."""
        strmap = {}
        for va, s in self.strings():
            if sub in s: strmap[va] = s
        if not strmap:
            if verbose: print(f"  no strings contain {sub!r}")
            return []
        res = []
        by_addr = collections.defaultdict(list)
        for where, addend in self.relocs:
            by_addr[addend].append(where)
        for nva, s in strmap.items():
            for slot in by_addr.get(nva, []):
                # scan +-0x40 around slot for a pointer into .text
                near = []
                for where, addend in self.relocs:
                    if abs(where - slot) <= 0x40 and self.t_start <= addend < self.t_end:
                        near.append((where - slot, where, addend))
                near.sort()
                if verbose:
                    print(f"  '{s[:70]}'")
                    print(f"    name@{hex(nva)}  table@{hex(slot)}")
                    for d, w, a in near:
                        print(f"       slot{d:+04x} {hex(w)} -> func {hex(a)}")
                if near:
                    best = min(near, key=lambda t: (t[0] < 0, abs(t[0])))
                    res.append((nva, s, slot, best[2]))
        return res

def main():
    db = SymDB()
    for q in (sys.argv[1:] or ['tickPlayerWaterGravity']):
        print(f"\n=== {q} ===")
        db.resolve(q)

if __name__ == '__main__':
    main()
