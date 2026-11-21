#!/usr/bin/env python3
"""
symfind.py -- Recover function addresses from Bedrock's embedded C++ system-name
strings. Bedrock's ECS keeps entt system metadata (mangled name + function
pointer) in .rodata; the registration site loads BOTH, so an ADRP+ADD xref to
a name string lets us read off the exact tick function address.

Usage: symfind.py <substring> [...]
"""
import sys, struct, re
sys.path.insert(0, 're')
from bedrock_re import Library
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

NAME_MIN = 8

class SymTable:
    def __init__(self, lib):
        self.lib = lib
        self.ro_start, self.ro_end, _ = lib.rodata_range()
        self.t_start, self.t_end = lib.text_range()[:2]
        self.ro = lib.read(self.ro_start, self.ro_end - self.ro_start)
        # index every NUL-terminated printable run in .rodata
        self.strs = []          # (va, text)
        self.by_start = {}
        i = 0; n = len(self.ro)
        run_start = None
        while i <= n:
            c = self.ro[i] if i < n else 0
            ok = 32 <= c < 127
            if ok and run_start is None: run_start = i
            elif not ok and run_start is not None:
                ln = i - run_start
                if ln >= NAME_MIN:
                    s = self.ro[run_start:i].decode('latin1')
                    va = self.ro_start + run_start
                    self.strs.append((va, s))
                    self.by_start[va] = s
                run_start = None
            i += 1
        print(f"[sym] {len(self.strs)} strings indexed in .rodata")

    def find(self, sub):
        return [(va, s) for va, s in self.strs if sub in s]

    def xref_sites(self, target_va, lib=None):
        """Scan .text for ADRP+ADD forming target_va. Returns list of insn vaddrs."""
        lib = lib or self.lib
        t_start, t_end, t_off = lib.text_range()
        lib.f.seek(t_off)
        blob = lib.f.read(t_end - t_start)
        out = []
        n = len(blob)
        for i in range(0, n - 8, 4):
            w = struct.unpack_from('<I', blob, i)[0]
            if (w & 0x9F000000) != 0x90000000:   # ADRP
                continue
            rd = w & 0x1F
            immlo = (w >> 29) & 3
            immhi = (w >> 5) & 0x7FFFF
            imm = (immhi << 2) | immlo
            if imm & (1 << 20): imm -= (1 << 21)
            page = ((t_start + i) & ~0xFFF) + (imm << 12)
            # look ahead up to 4 insns for ADD Xd, Xrd, #imm12
            for k in range(1, 5):
                j = i + 4*k
                if j + 4 > n: break
                w2 = struct.unpack_from('<I', blob, j)[0]
                if (w2 & 0xFFC00000) == 0x91000000:
                    rn = (w2 >> 5) & 0x1F
                    imm12 = (w2 >> 10) & 0xFFF
                    sh = (w2 >> 22) & 1
                    if rn == rd:
                        addr = page + (imm12 << (12 if sh else 0))
                        if addr == target_va:
                            out.append(t_start + i)
                        break
                # stop early if the base reg is clobbered
                if (w2 & 0xFFC00000) in (0x91000000, 0x11000000) and ((w2>>5)&0x1F)==rd and k>1:
                    break
        return out

    def funcs_near(self, insn_va, span=240):
        """Disassemble around a registration site, return .text ADRP+ADD targets
        (candidate function pointers)."""
        lib = self.lib
        lo = max(self.t_start, insn_va - span)
        blob = lib.read(lo, (insn_va - lo) + span)
        md = Cs(CS_ARCH_ARM64, CS_MODE_ARM); md.detail = False
        adr_page = {}
        found = []
        for ins in md.disasm(blob, lo):
            m = ins.mnemonic
            ops = ins.op_str
            if m == 'adrp':
                try:
                    rd, imm = ops.split(', ')
                    adr_page[rd.strip()] = int(imm.strip(), 16)
                except Exception: pass
            elif m == 'add' and '#' in ops:
                try:
                    parts = [p.strip() for p in ops.split(',')]
                    rd, rn = parts[0], parts[1]
                    imm = int(parts[2].lstrip('#'), 16)
                    if rn in adr_page:
                        tgt = adr_page[rn] + imm
                        if self.t_start <= tgt < self.t_end:
                            found.append((ins.address, tgt))
                except Exception: pass
        return found

def main():
    lib = Library('mods/apollon/game/libminecraftpe.so')
    st = SymTable(lib)
    for q in sys.argv[1:]:
        hits = st.find(q)
        print(f"\n=== {q}  ({len(hits)} strings) ===")
        for va, s in hits[:6]:
            print(f"  str {hex(va)}  len {len(s)}")
            print(f"      {s[:150]}")
            xs = st.xref_sites(va)
            print(f"      xrefs: {[hex(x) for x in xs]}")
            for x in xs[:4]:
                fp = st.funcs_near(x)
                for site, tgt in fp[:6]:
                    print(f"        site {hex(site)} -> func {hex(tgt)}")

if __name__ == '__main__':
    main()
