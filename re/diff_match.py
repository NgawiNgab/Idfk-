#!/usr/bin/env python3
"""diff_match.py -- cross-version function matcher for libminecraftpe.so.

Given the SAME set of 1.21.x offsets Apollon already ships, find the matching
function in a newer build.

Method (no symbols required on either side):
  1. Collect every function entry in the NEW binary from
     R_AARCH64_RELATIVE relocation addends inside .text (vtable/registry slots
     point at real entries), then extend by scanning for `stp x29,x30,[sp,#-N]!`
     prologues and `ret` boundaries.
  2. Hash the first K instructions as (mnemonic, normalised operands) where
     branch targets and pc-relative offsets are rewritten to *relative*
     displacements, so a function that moved does not change hash.
  3. Filter by body length within tolerance.
  4. Resolve: old offset -> best new candidate, with a confidence score.

Operand normalisation is what makes this work; a plain byte hash would match
nothing across versions.
"""
import sys, struct, hashlib, json, collections
sys.path.insert(0, 're')
from bedrock_re import Library
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

K_PROBE = 14          # instructions in the probe hash
LEN_TOL = 0.35        # allowed body-length difference

md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
md.detail = True

BRANCH = {'b','bl','cbz','cbnz','tbz','tbnz','blr','br','adrp','adr',
          'ldr','ldur','str','stur','prfm','beq','bne','bhs','blo','bhi','bls',
          'bge','blt','bgt','ble','bmi','bpl'}

BRANCHMN = {'b','bl','cbz','cbnz','tbz','tbnz','adr','adrp'}

def norm_ops(ins):
    """Branch targets -> opaque; data immediates keep their value (so
    `cmp #1` never matches `cmp #7`). Memory operands keep base+disp."""
    out = []
    isbranch = ins.mnemonic in BRANCHMN
    for op in ins.operands:
        if op.type == 2:                      # ARM64_OP_IMM
            out.append('TGT' if isbranch else f"#{op.imm}")
        elif op.type == 3:                    # ARM64_OP_MEM
            m = op.mem
            base = ins.reg_name(m.base) if m.base else '?'
            idx  = ins.reg_name(m.index) if m.index else ''
            out.append(f"MEM:{base}+{idx}+{m.disp}")
        else:
            out.append(ins.reg_name(op.reg) if op.type == 1 else '?')
    return ','.join(out)

def insn_sig(ins):
    return ins.mnemonic + ' ' + norm_ops(ins)

class Bin:
    def __init__(self, path):
        self.lib = Library(path)
        self.t0, self.t1, self.toff = self.lib.text_range()
        self.lib.f.seek(self.toff)
        self.blob = self.lib.f.read(self.t1 - self.t0)
        self.n = len(self.blob)//4
        self.words = struct.unpack_from('<%dI' % self.n, self.blob, 0)
        self._entry_cache = {}

    def insn_bytes(self, va, count):
        start = va - self.t0
        if start < 0 or start + count*4 > len(self.blob): return None
        return self.blob[start:start+count*4]

    def probe(self, va, k=K_PROBE):
        data = self.insn_bytes(va, k)
        if not data: return None
        sigs = []
        for i, ins in enumerate(md.disasm(data, va)):
            sigs.append(insn_sig(ins))
            if len(sigs) >= k: break
        if not sigs: return None
        return hashlib.blake2b('\n'.join(sigs).encode(), digest_size=8).hexdigest()

    def entry_of(self, va, back=1500):
        """Nearest function prologue at or before va (inclusive)."""
        if va in self._entry_cache: return self._entry_cache[va]
        cur = va
        res = va
        while cur - self.t0 >= 0 and (va - cur) < back*4:
            x = self.words[(cur - self.t0)//4]
            # STP x29,x30,[sp,#-imm]!  (pre-index, 64-bit)
            # SUB sp, sp, #imm         (0xD10003FF | imm12<<10)
            if (x & 0xFFC07FFF) == 0xA9807BFD or (x & 0xFFC003FF) == 0xD10003FF:
                res = cur
                break
            cur -= 4
        self._entry_cache[va] = res
        return res

    def body_len(self, va, limit=4000):
        data = self.insn_bytes(va, limit)
        if not data: return 0
        n = 0
        for ins in md.disasm(data, va):
            n += 1
            if ins.mnemonic in ('ret','br') or (ins.mnemonic=='b' and n > 2):
                break
        return n

    def entries(self, addends):
        """Plausible entry points: reloc addends in .text + prologue scan."""
        ents = set(a for a in addends if self.t0 <= a < self.t1)
        # prologue filter: stp x29,x30,[sp,#-imm]!  == 0xA9Bxxxxx family
        for i in range(self.n-1):
            x = self.words[i]
            # STP (pre-index, 64-bit): 1010100110 imm7 Rt2 Rn Rt
            if (x & 0xFFC00000) != 0xA9800000: continue
            if (x >> 22) & 1: continue              # not post-index
            rt  = x & 0x1F
            rn  = (x >> 5) & 0x1F
            rt2 = (x >> 10) & 0x1F
            if rt == 29 and rt2 == 30 and rn == 31:
                ents.add(self.t0 + i*4)
        # SUB sp, sp, #imm  -> frameless / large-frame prologue
        for i in range(self.n):
            if (self.words[i] & 0xFFC003FF) == 0xD10003FF:
                ents.add(self.t0 + i*4)
        return ents

if __name__ == '__main__':
    print("module ok")

# ---------------------------------------------------------------------------
# resolver
# ---------------------------------------------------------------------------
def build_index(b, entries=None, k=K_PROBE):
    from symtable import SymDB
    if entries is None:
        db = SymDB(b.lib.path)
        entries = sorted({a for _, a in db.relocs if b.t0 <= a < b.t1})
    idx = {}
    for e in entries:
        h = b.probe(e, k)
        if h:
            idx.setdefault(h, []).append(e)
    return idx

def full_sig(b, entry, limit=240):
    data = b.insn_bytes(entry, limit)
    if not data: return ''
    out = []
    for i, ins in enumerate(md.disasm(data, entry)):
        out.append(insn_sig(ins))
        if ins.mnemonic in ('ret','br') or (ins.mnemonic == 'b' and i > 1):
            break
    return '\n'.join(out)

def resolve(b_old, b_new, idx_new, old_off, k=K_PROBE):
    """Return ranked candidates for old_off in the new binary."""
    ent_old = b_old.entry_of(old_off)
    h = b_old.probe(ent_old, k)
    if not h: return []
    cands = idx_new.get(h, [])
    if not cands: return []
    sig_old = full_sig(b_old, ent_old)
    scored = []
    for c in cands:
        sig_new = full_sig(b_new, c)
        # similarity on the full body signature
        a = sig_old.split('\n'); bb = sig_new.split('\n')
        common = sum(1 for x, y in zip(a, bb) if x == y)
        denom = max(len(a), 1)
        scored.append((common/denom, c))
    scored.sort(reverse=True)
    delta = old_off - ent_old
    return [(conf, c, c + delta) for conf, c in scored]
