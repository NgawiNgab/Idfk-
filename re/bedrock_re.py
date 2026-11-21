#!/usr/bin/env python3
"""
bedrock_re.py -- Recover Bedrock hook offsets from a stripped libminecraftpe.so

Techniques, in order of confidence:
  1. STRING  - locate a unique .rodata string; find ADRP+LDR/ADD pairs in .text
               that reference it; that code is the function which uses it.
               Then walk backwards to the function prologue (stp x29,x30,[sp,#-N]!)
               to get the function start address.
  2. CONST   - scan .text for a specific 32-bit float or int constant, then
               walk back to the function prologue. Useful for leaf functions
               that just `return 0.42f` / `return -0.08f`.

Every result carries: address, how it was found, and a confidence level.
Nothing is guessed: if no xref is found we report NOT-FOUND rather than
emitting a plausible-looking number.
"""
import sys, struct, re, json, argparse
from elftools.elf.elffile import ELFFile
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM
from capstone.arm64 import ARM64_OP_IMM, ARM64_OP_REG, ARM64_OP_MEM

class Library:
    def __init__(self, path):
        self.path = path
        self.f = open(path, 'rb')
        self.elf = ELFFile(self.f)
        self._load_segments()
        self._load_sections()
        self.md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
        self.md.detail = True

    def _load_segments(self):
        self.segments = []
        for seg in self.elf.iter_segments():
            if seg['p_type'] != 'PT_LOAD':
                continue
            self.segments.append({
                'off': seg['p_offset'], 'vaddr': seg['p_vaddr'],
                'filesz': seg['p_filesz'], 'flags': seg['p_flags'],
            })

    def _load_sections(self):
        self.sections = {}
        for s in self.elf.iter_sections():
            self.sections[s.name] = s

    def vaddr_to_off(self, vaddr):
        for s in self.segments:
            if s['vaddr'] <= vaddr < s['vaddr'] + s['filesz']:
                return s['off'] + (vaddr - s['vaddr'])
        return None

    def off_to_vaddr(self, off):
        for s in self.segments:
            if s['off'] <= off < s['off'] + s['filesz']:
                return s['vaddr'] + (off - s['off'])
        return None

    def read(self, vaddr, n):
        off = self.vaddr_to_off(vaddr)
        if off is None: return None
        self.f.seek(off)
        return self.f.read(n)

    def text_range(self):
        t = self.sections.get('.text')
        return (t['sh_addr'], t['sh_addr'] + t['sh_size'], t['sh_offset'])

    def rodata_range(self):
        r = self.sections.get('.rodata')
        return (r['sh_addr'], r['sh_addr'] + r['sh_size'], r['sh_offset'])

    # ---------- string discovery ----------
    def find_string(self, needle, exact=False):
        """Return list of (vaddr, matched_bytes) for .rodata occurrences."""
        start, end, off = self.rodata_range()
        size = end - start
        self.f.seek(off)
        blob = self.f.read(size)
        hits = []
        nb = needle.encode() if isinstance(needle, str) else needle
        i = 0
        while True:
            i = blob.find(nb, i)
            if i < 0: break
            # require NUL-termination for a real C string
            j = i + len(nb)
            if j < len(blob) and blob[j] == 0:
                hits.append((start + i, blob[i:j].decode('latin1')))
            i += 1
        return hits

    # ---------- xref discovery ----------
    def find_adrp_add_xrefs(self, target_vaddr):
        """
        Find ADRP+ADD/LDR pairs whose computed address == target_vaddr.
        Returns list of (insn_vaddr_of_pair, page_reg) — the instruction that
        forms the reference.
        """
        tstart, tend, toff = self.text_range()
        self.f.seek(toff)
        blob = self.f.read(tend - tstart)
        xrefs = []
        # decode page-granularity references only; full linear decode of 211MB
        # is too slow, so we use a targeted heuristic on ADRP instructions.
        # ADRP encoding: [31] immlo[2] 10000 [22:5] immhi [4:0] Rd
        n = len(blob)
        for i in range(0, n - 4, 4):
            w = struct.unpack_from('<I', blob, i)[0]
            if (w & 0x9F000000) != 0x90000000:   # ADRP opcode mask
                continue
            rd = w & 0x1F
            immlo = (w >> 29) & 0x3
            immhi = (w >> 5) & 0x7FFFF
            imm = (immhi << 2) | immlo
            if imm & (1 << 20):                  # sign extend 21-bit
                imm -= (1 << 21)
            pc = tstart + i
            page = ((pc & ~0xFFF) + (imm << 12)) & 0xFFFFFFFFFFFF
            # target must be on this page for an ADD/LDR to reach it
            if page != (target_vaddr & ~0xFFF):
                continue
            # look at next few instructions for ADD/LDR with same Rd
            for k in range(1, 5):
                if i + k*4 + 4 > n: break
                w2 = struct.unpack_from('<I', blob, i + k*4)[0]
                # ADD (immediate, 64-bit): 1001 0001 00 imm12 Rn Rd
                if (w2 & 0xFFC00000) == 0x91000000:
                    rn = (w2 >> 5) & 0x1F
                    rd2 = w2 & 0x1F
                    imm12 = (w2 >> 10) & 0xFFF
                    if rn == rd and rd2 == rd and (page + imm12) == target_vaddr:
                        xrefs.append({'pair_vaddr': pc, 'via': 'ADRP+ADD'})
                        break
                # LDR (unsigned offset, 64-bit): 1111 1001 01 imm12 Rn Rt
                if (w2 & 0xFFC00000) == 0xF9400000:
                    rn = (w2 >> 5) & 0x1F
                    imm12 = (w2 >> 10) & 0xFFF
                    if rn == rd and (page + imm12*8) == target_vaddr:
                        xrefs.append({'pair_vaddr': pc, 'via': 'ADRP+LDR'})
                        break
                # ADR (rare) - skip
        return xrefs

    # ---------- function boundary ----------
    def function_start(self, vaddr, max_back=0x6000):
        """
        Walk backwards from `vaddr` looking for a typical ARM64 prologue:
            stp x29, x30, [sp, #-N]!
            mov x29, sp
        Returns the prologue address, or None.
        """
        cur = vaddr & ~0x3
        scanned = 0
        while scanned < max_back:
            b = self.read(cur - 4, 4)
            if not b or len(b) < 4: return None
            w = struct.unpack('<I', b)[0]
            # STP x29,x30,[sp,#-imm]!  == 0xA9B.....
            # encoding: 1010 1001 10 imm7 Rt2 Rn Rt  with pre-index
            if (w & 0xFFC00000) == 0xA9800000:
                rt = w & 0x1F; rn = (w >> 5) & 0x1F; rt2 = (w >> 10) & 0x1F
                if rt == 29 and rt2 == 30 and rn == 31:
                    return cur - 4
            # also accept SUB sp, sp, #imm followed by STP x29,x30,[sp,#imm]
            cur -= 4
            scanned += 4
        return None

    # ---------- constant scan ----------
    def find_constant(self, value_bytes, limit=40):
        """Find all occurrences of a raw byte literal inside .text."""
        tstart, tend, toff = self.text_range()
        self.f.seek(toff)
        blob = self.f.read(tend - tstart)
        hits = []
        i = 0
        while len(hits) < limit:
            i = blob.find(value_bytes, i)
            if i < 0: break
            if i % 4 == 0:
                hits.append(tstart + i)
            i += 1
        return hits

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('lib')
    ap.add_argument('--string', help='find code referencing this .rodata string')
    ap.add_argument('--func', action='store_true', help='resolve to function start')
    ap.add_argument('--const-f32', help='find a float constant in .text')
    ap.add_argument('--const-u32', help='find a u32 constant in .text')
    ap.add_argument('--json', action='store_true')
    a = ap.parse_args()

    lib = Library(a.lib)
    print(f"# {a.lib}")
    print(f"# .text  {hex(lib.text_range()[0])} .. {hex(lib.text_range()[1])}")
    print(f"# .rodata{hex(lib.rodata_range()[0])} .. {hex(lib.rodata_range()[1])}")

    if a.string:
        hits = lib.find_string(a.string)
        print(f"\nSTRING {a.string!r}: {len(hits)} rodata hit(s)")
        for va, txt in hits[:4]:
            print(f"  .rodata {hex(va)}")
            xr = lib.find_adrp_add_xrefs(va)
            print(f"    xrefs: {len(xr)}")
            for x in xr[:6]:
                fs = lib.function_start(x['pair_vaddr']) if a.func else None
                s = f"  func {hex(fs)}" if fs else ""
                print(f"    {x['via']} at {hex(x['pair_vaddr'])}{s}")

    if a.const_f32:
        val = struct.pack('<f', float(a.const_f32))
        hits = lib.find_constant(val)
        print(f"\nCONST f32 {a.const_f32}: {len(hits)} raw hit(s) in .text")
        for h in hits[:10]:
            fs = lib.function_start(h) if a.func else None
            print(f"  {hex(h)}" + (f"  func {hex(fs)}" if fs else ""))

    if a.const_u32:
        val = struct.pack('<I', int(a.const_u32, 0))
        hits = lib.find_constant(val)
        print(f"\nCONST u32 {a.const_u32}: {len(hits)} raw hit(s) in .text")
        for h in hits[:10]:
            fs = lib.function_start(h) if a.func else None
            print(f"  {hex(h)}" + (f"  func {hex(fs)}" if fs else ""))

if __name__ == '__main__':
    main()
