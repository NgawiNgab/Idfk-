#!/usr/bin/env python3
"""Fast AArch64 adrp/add+ldr xref finder for a huge ELF.

Answers: which code addresses materialise this target VA (via adrp+add)?
Also: which .data.rel.ro R_AARCH64_RELATIVE slots point at a given VA?
"""
import sys, struct
from elftools.elf.elffile import ELFFile

class Bin:
    def __init__(self, path):
        self.f = open(path,'rb')
        self.elf = ELFFile(self.f)
        self.secs = {}
        for s in self.elf.iter_sections():
            try:
                if s['sh_type'] == 'SHT_NOBITS' or s['sh_size'] == 0:
                    continue
            except Exception:
                continue
            self.secs[s.name] = (s['sh_addr'], s['sh_offset'], s['sh_size'])
        # quick vaddr -> file offset
        self.loads=[]
        for seg in self.elf.iter_segments():
            if seg['p_type'] == 'PT_LOAD':
                self.loads.append((seg['p_vaddr'], seg['p_offset'], seg['p_filesz']))
        self.loads.sort()

    def read(self, vaddr, n):
        for va, off, sz in self.loads:
            if va <= vaddr and vaddr+n <= va+sz:
                self.f.seek(off + (vaddr-va))
                return self.f.read(n)
        raise ValueError(f"unmapped {vaddr:#x}")

    def u32(self, vaddr):
        return struct.unpack('<I', self.read(vaddr,4))[0]

def adrp_pages(b: Bin, sec='.text'):
    """yield (insn_addr, page_va) for every adrp in .text"""
    addr, off, size = b.secs[sec]
    b.f.seek(off)
    buf = b.f.read(size)
    n = size//4
    out = []
    for i in range(n):
        w = struct.unpack_from('<I', buf, i*4)[0]
        if (w & 0x9F000000) == 0x90000000:      # adrp
            immhi = (w >> 5) & 0x7FFFF
            immlo = (w >> 29) & 0x3
            imm = (immhi << 2) | immlo
            if imm & (1<<20): imm -= (1<<21)
            base = addr + i*4
            page = (base & ~0xFFF) + (imm << 12)
            out.append((base, w & 0x1F, page, buf, i))
    return out, buf, addr

def find_adrp_add(b: Bin, target, sec='.text'):
    """Return list of code addrs where adrp+add (next insn) resolves to target."""
    hits = []
    cands, buf, base_addr = adrp_pages(b, sec)
    tpage = target & ~0xFFF
    toff  = target & 0xFFF
    for (caddr, rd, page, _buf, i) in cands:
        if page != tpage: continue
        # look at next up to 3 instructions for add rd, rd, #toff  (toff must fit shift0 imm12)
        for k in range(1,4):
            j = i+k
            if j*4+4 > len(buf): break
            w = struct.unpack_from('<I', buf, j*4)[0]
            # ADD (immediate) 64-bit: sf=1 op=0 S=0 100010 sh imm12 Rn Rd
            if (w & 0x7F800000) == 0x11000000:
                sh = (w>>22)&1
                imm12 = (w>>10)&0xFFF
                rn = (w>>5)&0x1F
                rdd = w & 0x1F
                val = imm12 << (12 if sh else 0)
                if rn == rd and rdd == rd and val == toff:
                    hits.append(caddr)
                    break
    return hits

def relocs_into(b: Bin, target):
    """R_AARCH64_RELATIVE entries whose addend == target"""
    out=[]
    for secname, (addr, off, size) in b.secs.items():
        pass
    rela = None
    for s in b.elf.iter_sections():
        if s['sh_type'] == 'SHT_RELA':
            rela = s
    res=[]
    for s in b.elf.iter_sections():
        if s['sh_type'] != 'SHT_RELA': continue
        tgt = s['sh_info']
        for r in s.iter_relocations():
            if r['r_info_type'] != 1027:  # R_AARCH64_RELATIVE
                continue
            if r['r_addend'] == target:
                res.append((s.name, r['r_offset']))
    return res

if __name__ == '__main__':
    b = Bin(sys.argv[1])
    mode = sys.argv[2]
    tgt = int(sys.argv[3], 0)
    if mode == 'xref':
        h = find_adrp_add(b, tgt)
        print(f"adrp+add xrefs to {tgt:#x}: {len(h)}")
        for a in h[:40]: print(f"  {a:#x}")
    elif mode == 'reloc':
        r = relocs_into(b, tgt)
        print(f"RELATIVE relocs with addend {tgt:#x}: {len(r)}")
        for nm,o in r[:40]: print(f"  {nm} slot {o:#x}")
