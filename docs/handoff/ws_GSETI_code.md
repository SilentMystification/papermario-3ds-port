# ws_GSETI code (exact text for ws_GSETI.md; delete with that page)
## A: gset.py move_blocks (replaces the whole function)
```python
def move_blocks(root, src, ok, twin_dir, assets=None):
    """Block mode. Returns (moved names, the twin path or None).
    GSETI: an asset include (`#include "<inc>.inc.c"`, assets/us/<inc> defines Gfx lists, all ok) of the block source moves too. The device
    file keeps one `NDN_DECL(X);` line per list of the include in its place. The twin gets the include line in file order."""
    p = root / src
    lines = p.read_text(encoding="utf-8").split("\n")
    ns = file_ns(p)
    tok = {}
    blocks = find_blocks(lines, ns, tok)
    T = lambda n: tok.get(n, n)
    W = lambda n: r"(?<!\w)" + re.escape(n) + r"(?!\w)"
    moving = {n for n, _, _ in blocks if n in ok}
    in_block = set()
    for _, s, e in blocks:
        in_block.update(range(s, e + 1))
    incs = {}  # GSETI: line index -> (include path, list names, asset text)
    if INC_BLOCK and assets is not None:
        for k, ln in enumerate(lines):
            m = INC.match(ln)
            if not m or k in in_block or not (assets / m.group(2)).is_file():
                continue
            at = (assets / m.group(2)).read_text(encoding="utf-8", errors="replace")
            names = gfx_names(at)
            if names and all(n in ok for n in names):
                incs[k] = (m.group(2), names, at)
    inc_moving = set(incs)
    if HOLD_RULE:
        for k, ln in enumerate(lines):
            if k in in_block or k in incs or DECL.match(ln) or NDECL.match(ln):
                continue
            hit = {n for n in moving if re.search(W(T(n)), ln)}
            if hit and hold_line(ln, {T(n) for n in hit}):
                moving -= hit
            for ik in list(inc_moving):
                ihit = {n for n in incs[ik][1] if re.search(W(n), ln)}
                if ihit and hold_line(ln, ihit):
                    inc_moving.discard(ik)
    while True:
        inside = set()
        for n, s, e in blocks:
            if n in moving:
                inside.update(range(s, e + 1))
        unmoved = "\n".join([lines[k] for k in sorted(in_block) if k not in inside] + [incs[k][2] for k in sorted(incs) if k not in inc_moving])
        bad = {n for n in moving if re.search(W(T(n)), unmoved)}
        ibad = {k for k in inc_moving if any(re.search(W(n), unmoved) for n in incs[k][1])}
        if not bad and not ibad:
            break
        moving -= bad
        inc_moving -= ibad
    if not moving and not inc_moving:
        return [], None
    out, body, moved, k = [], [], [], 0
    starts = {s: (n, e) for n, s, e in blocks if n in moving}
    while k < len(lines):
        if k in starts:
            n, e = starts[k]
            out.append(f"NDN_DECL({T(n)});")
            body.extend(lines[k:e + 1])
            moved.append(n)
            k = e + 1
            continue
        if k in inc_moving:
            inc, names, _ = incs[k]
            out.extend(f"NDN_DECL({n});" for n in names)
            if f'#include "{inc}"' not in body:  # an include in two #if branches (level_up.c VERSION_JP) goes to the twin once
                body.append(f'#include "{inc}"')
            moved.extend(names)
            k += 1
            continue
        out.append(lines[k])
        k += 1
    if "nd_gtab.h" not in "\n".join(lines):
        out = add_gtab(out)
    p.write_text("\n".join(out), encoding="utf-8")
    twin = root / twin_dir / src
    twin.parent.mkdir(parents=True, exist_ok=True)
    old = twin.read_text(encoding="utf-8").split("\n") if twin.exists() else [f'#include "{src}"']
    tbody = old[:1] + [x for x in old[1:] if x] + body
    twin.write_text("\n".join(tbody) + "\n", encoding="utf-8")
    for n in moved:
        if n in tok:
            NSN[n] = (tok[n], ns, p.parent)
    return moved, twin
```
## B: 3ds/tools/gconv_src.d/GSETI.txt
```
src/battle/area/kzn2/anim/unused_gfx.c
src/battle/move/hammer/hammer_throw.c
src/i_spy.c
src/inspect_icon.c
src/pulse_stone.c
src/speech_bubble.c
src/status_star_shimmer.c
src/world/area_iwa/iwa_01/splash.c
src/world/area_kmr/kmr_20/records.c
src/world/area_kmr/kmr_22/npc.c
src/world/area_kzn/kzn_19/npc.c
src/world/area_pra/pra_31/stairs_skele.c
src/world/area_pra/pra_31/texture_1.c
src/world/area_pra/pra_31/texture_2.c
src/world/area_pra/pra_31/texture_3.c
src/world/area_sam/sam_05/monstar_gfx.c
src/pause/pause_styles.c
src/filemenu/filemenu_styles.c
src/world/area_mgm/mgm_00/scoreboard.c
src/world/area_hos/hos_05/intro.c
src/world/area_kmr/kmr_23/npc.c
src/world/area_kkj/kkj_19/main.c
src/world/area_pra/pra_31/stairs_lights.c
src/world/area_pra/pra_31/stairs_vtx.c
src/world/area_sam/sam_05/monstar_vtx.c
```
## C: 3ds/tools/gset.d/GSETI.txt
```
src/battle/area/kzn2/anim/unused_gfx.c
src/battle/move/hammer/hammer_throw.c
src/i_spy.c
src/inspect_icon.c
src/pulse_stone.c
src/speech_bubble.c
src/status_star_shimmer.c
src/world/area_iwa/iwa_01/splash.c
src/world/area_kmr/kmr_20/records.c
src/world/area_kmr/kmr_22/npc.c
src/world/area_kzn/kzn_19/npc.c
src/world/area_pra/pra_31/stairs_skele.c
src/world/area_pra/pra_31/texture_1.c
src/world/area_pra/pra_31/texture_2.c
src/world/area_pra/pra_31/texture_3.c
src/world/area_sam/sam_05/monstar_gfx.c
src/pause/pause_styles.c
src/filemenu/filemenu_styles.c
src/world/area_mgm/mgm_00/scoreboard.c
src/world/area_hos/hos_05/intro.c
src/world/area_kmr/kmr_23/npc.c
src/world/area_kkj/kkj_19/main.c
src/world/area_pra/pra_31/stairs_lights.c
src/world/area_pra/pra_31/stairs_vtx.c
src/world/area_sam/sam_05/monstar_vtx.c
src/starpoint.c
src/level_up.c
src/world/area_tst/tst_13/main.c
src/world/area_dgb/dgb_01/img0.c
src/world/area_dgb/dgb_01/img1.c
src/world/area_dgb/dgb_01/img2.c
src/world/area_dgb/dgb_01/img3.c
src/world/area_dgb/dgb_01/img4.c
src/world/area_dgb/dgb_01/img5.c
src/world/area_dgb/dgb_01/img6.c
src/world/area_dgb/dgb_01/img7.c
```
