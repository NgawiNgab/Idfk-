#!/usr/bin/env python3
"""
syms.py -- Resolve Bedrock offsets from the .dynsym symbol table.

libminecraftpe.so keeps a full 91k-entry .dynsym (79k STT_FUNC). Every offset
Apollon needs is a named C++ function, so resolution is exact rather than
heuristic: demangle, match, done.
"""
import sys, re, subprocess, json, collections
sys.path.insert(0,'re')
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

PATH = 'mods/apollon/game/libminecraftpe.so'

class Syms:
    def __init__(self, path=PATH):
        self.elf = ELFFile(open(path,'rb'))
        self.by_name = {}
        self.by_addr = collections.defaultdict(list)
        self.funcs = []
        for sec in self.elf.iter_sections():
            if not isinstance(sec, SymbolTableSection): continue
            if sec.name not in ('.dynsym','.symtab'): continue
            for s in sec.iter_symbols():
                if not s.name: continue
                self.by_name.setdefault(s.name, []).append((s['st_value'], s['st_size'], s['st_info']['type']))
                if s['st_info']['type']=='STT_FUNC' and s['st_value']:
                    self.by_addr[s['st_value']].append(s.name)
                    self.funcs.append((s['st_value'], s['st_size'], s.name))
        print(f"[syms] {len(self.by_name)} distinct names, {len(self.funcs)} function symbols")

    def addr(self, name):
        e = self.by_name.get(name)
        if not e: return None
        return e[0][0]

    def find(self, pat):
        rx = re.compile(pat)
        return [n for n in self.by_name if rx.search(n)]

if __name__ == '__main__':
    s = Syms()
    if len(sys.argv) > 1:
        for pat in sys.argv[1:]:
            hits = s.find(pat)
            print(f"\n=== /{pat}/  ({len(hits)})")
            for n in sorted(hits)[:30]:
                v,sz,t = s.by_name[n][0]
                print(f"   {v:#012x}  size {sz:<8} {t:<10} {n[:130]}")
    else:
        for demo in ('MobMovementGravity','CameraBobSystem4tick','GameMode5_attack',
                     'Actor9getHealth','Player9isEmoting','LocalPlayer10normalTick'):
            print(f"\n=== {demo} ===")
            for n in sorted(s.find(demo))[:12]:
                v,sz,t = s.by_name[n][0]
                print(f"   {v:#012x} {sz:<7} {n[:120]}")
