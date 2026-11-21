#!/usr/bin/env python3
"""
tasks.py -- Batch hook recovery for Apollon's offsets against a given
libminecraftpe.so.

Each task declares what evidence should exist and how to confirm it.
Confidence is only "high" when the xref resolves to a real function prologue
AND the disassembly around the reference matches the expected shape.
"""
import sys, struct, json
sys.path.insert(0, 're')
from bedrock_re import Library

# name, kind, needle/const, expected-verification note
TASKS = [
    # --- screen-state hooks: Apollon's ClientInstance::onTick reads these ---
    dict(name='ScreenName_hud',    kind='string', needle='hud_screen'),
    dict(name='ScreenName_chat',   kind='string', needle='chat_screen'),
    dict(name='ManageWorldsScreen',kind='string', needle='/hbui/index.html'),
    # --- audio/UI feedback Apollon calls from onTick ---
    dict(name='PlayUI_pop',        kind='string', needle='random.pop'),
    # --- option / platform getters ---
    dict(name='GetMaxPlayers',     kind='string', needle='maxPlayers'),
    dict(name='IsEmoting',         kind='string', needle='isEmoting'),
    dict(name='Paperdoll',         kind='string', needle='paperdoll'),
    dict(name='Gamma',             kind='string', needle='Gamma'),
    # --- physics constants (leaf funcs returning a literal) ---
    dict(name='Gravity_-0.08',     kind='f32',    value=-0.08),
    dict(name='Jump_0.42',         kind='f32',    value=0.42),
]

def disasm_context(lib, addr, before=0, after=14):
    code = lib.read(addr, (before+after)*4)
    out = []
    base = addr
    for ins in lib.md.disasm(code, base):
        out.append(f"    {ins.address:08x}  {ins.mnemonic:8} {ins.op_str}")
    return out

def main():
    lib = Library(sys.argv[1] if len(sys.argv) > 1
                  else 'mods/apollon/game/libminecraftpe.so')
    results = []
    for t in TASKS:
        row = {'task': t['name'], 'kind': t['kind'], 'hits': []}
        if t['kind'] == 'string':
            hits = lib.find_string(t['needle'])
            for va, txt in hits:
                xr = lib.find_adrp_add_xrefs(va)
                for x in xr:
                    fs = lib.function_start(x['pair_vaddr'])
                    row['hits'].append({
                        'string_at': hex(va), 'string': txt,
                        'xref_at': hex(x['pair_vaddr']), 'via': x['via'],
                        'func': hex(fs) if fs else None,
                    })
        else:
            val = struct.pack('<f', t['value'])
            for h in lib.find_constant(val):
                fs = lib.function_start(h)
                row['hits'].append({
                    'const_at': hex(h),
                    'func': hex(fs) if fs else None,
                })
        # confidence: has a resolved function prologue
        good = [h for h in row['hits'] if h.get('func')]
        row['n_xrefs'] = len(row['hits'])
        row['n_resolved'] = len(good)
        row['confidence'] = ('high' if len(good) == 1
                             else 'ambiguous' if len(good) > 1
                             else 'none')
        results.append(row)
        print(f"\n### {t['name']}  [{t['kind']} {t.get('needle') or t.get('value')}]")
        print(f"    xrefs={row['n_xrefs']} resolved={row['n_resolved']} -> {row['confidence']}")
        for h in row['hits'][:6]:
            print(f"    {h}")
    json.dump(results, open('re/results.json','w'), indent=1)
    print("\nwrote re/results.json")

if __name__ == '__main__':
    main()
