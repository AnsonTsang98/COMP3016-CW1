import math, os, struct
import make_items as mi   # re-uses the six 32x32 item drawings
from make_items import ITEMS, PAL, W, H, KEY, write_png

FRAMES = 8
SHINE = 'c'   # cream: the only colour added for the effects (never pure white)


def bbox(rows):
    xs = [x for r in rows for x, c in enumerate(r) if c != '.']
    ys = [y for y, r in enumerate(rows) if any(c != '.' for c in r)]
    return min(xs), min(ys), max(xs), max(ys)


def make_frame(rows, f):
    g = [list(r) for r in rows]
    x0, y0, x1, y1 = bbox(rows)
    w, h = x1 - x0 + 1, y1 - y0 + 1

    # diagonal glint sweeping bottom-left/top-right "/" streak, frames 3..7
    if 3 <= f <= 7:
        dmin, dmax = x0 + y0 + 2, x1 + y1 - 2
        d = dmin + (dmax - dmin) * (f - 3) // 4
        for y in range(H):
            for x in range(W):
                if g[y][x] not in '.k' and d <= x + y <= d + 1:
                    g[y][x] = SHINE

    # twinkling plus-shaped sparkle
    size = [0, 1, 2, 3, 2, 1, 0, 0][f]
    ax = x0 + int(w * 0.7)
    ay = y0 + max(3, h // 4) + 1
    if size >= 1:
        g[ay][ax] = SHINE
    if size >= 2:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            g[ay + dy][ax + dx] = SHINE
    if size >= 3:
        for dx, dy in ((2, 0), (-2, 0), (0, 2), (0, -2)):
            g[ay + dy][ax + dx] = SHINE

    # floating: whole-pixel vertical bob
    dy = -round(2 * math.sin(2 * math.pi * f / FRAMES))
    out = [['.'] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            if g[y][x] != '.':
                assert 0 <= y + dy < H, "bob pushed pixels off the frame"
                out[y + dy][x] = g[y][x]
    return [''.join(r) for r in out]


def write_sheet_bmp(path, rows):
    sw, sh = len(rows[0]), len(rows)
    assert sw % 4 == 0
    used = sorted({c for r in rows for c in r}, key=lambda c: (c != '.', c))
    assert used[0] == '.'
    idx = {c: i for i, c in enumerate(used)}
    pal = [PAL[c] for c in used]
    off = 14 + 40 + 4 * len(pal)
    hdr = b'BM' + struct.pack('<IHHI', off + sw * sh, 0, 0, off)
    info = struct.pack('<IiiHHIIiiII', 40, sw, sh, 1, 8, 0, sw * sh, 2835, 2835,
                       len(pal), len(pal))
    pal_b = b''.join(bytes((b, g, r, 0)) for r, g, b in pal)
    body = b''.join(bytes(idx[c] for c in rows[y]) for y in range(sh - 1, -1, -1))
    with open(path, 'wb') as fh:
        fh.write(hdr + info + pal_b + body)
    return len(pal)


os.makedirs('/home/claude/anim', exist_ok=True)
all_frames = {}
for name, rows in ITEMS.items():
    frames = [make_frame(rows, f) for f in range(FRAMES)]
    assert len({tuple(fr) for fr in frames}) == FRAMES, name   # all frames differ
    sheet = [''.join(frames[f][y] for f in range(FRAMES)) for y in range(H)]
    n = write_sheet_bmp(f'/home/claude/anim/{name}_anim.bmp', sheet)
    assert n <= 8, (name, n)
    for r in sheet:                       # key colour only used as background
        for c in r:
            assert c == '.' or PAL[c] != KEY
    all_frames[name] = frames
    print(f'{name}_anim.bmp: 256x32, {n} palette entries incl. transparent key')

# preview: one strip per item, 3x scale
BG = (200, 214, 230)
S, GAP = 3, 4
names = list(all_frames)
cw = FRAMES * W + (FRAMES + 1) * GAP
ch = len(names) * H + (len(names) + 1) * GAP
canvas = [[BG] * cw for _ in range(ch)]
for i, nm in enumerate(names):
    for f, fr in enumerate(all_frames[nm]):
        ox, oy = GAP + f * (W + GAP), GAP + i * (H + GAP)
        for y in range(H):
            for x in range(W):
                c = fr[y][x]
                if c != '.':
                    canvas[oy + y][ox + x] = PAL[c]
big = []
for row in canvas:
    wide = [p for p in row for _ in range(S)]
    big.extend([wide] * S)
write_png('/home/claude/anim/preview_strips.png', big)

# optional animated GIF preview if Pillow happens to be installed
try:
    from PIL import Image
    gs = 4
    gifs = []
    for f in range(FRAMES):
        im = Image.new('RGB', (len(names) * (W + GAP) * gs + GAP * gs, (H + 2 * GAP) * gs), BG)
        for i, nm in enumerate(names):
            fr = all_frames[nm][f]
            for y in range(H):
                for x in range(W):
                    c = fr[y][x]
                    if c != '.':
                        px = (GAP + i * (W + GAP) + x) * gs
                        py = (GAP + y) * gs
                        im.paste(PAL[c], (px, py, px + gs, py + gs))
        gifs.append(im)
    gifs[0].save('/home/claude/anim/preview.gif', save_all=True,
                 append_images=gifs[1:], duration=100, loop=0)
    print('gif written')
except ImportError:
    print('Pillow not available, no gif')
