import math, os, struct
import make_chars as mc
from make_chars import Sprite, PAL_MAIN, PAL_ZOMBIE, PAL_RAGE, W, H, KEY, write_png

FRAMES = 8
OUT = '/home/claude/char_anims'
os.makedirs(OUT, exist_ok=True)


def lerp(a, b, t):
    return a + (b - a) * t


def lr(a, b, t):
    return round(lerp(a, b, t))


def clamp(x, lo=0.0, hi=1.0):
    return max(lo, min(hi, x))


# ================================================================== walking (8 frames)
WALKS = [
    ('main_pistol_walk', mc.main_pistol, PAL_MAIN),
    ('main_rifle_walk', mc.main_rifle, PAL_MAIN),
    ('main_shotgun_walk', mc.main_shotgun, PAL_MAIN),
    ('main_smg_walk', mc.main_smg, PAL_MAIN),
    ('zombie_idle_walk', mc.zombie_idle, PAL_ZOMBIE),
    ('zombie_chase_walk', mc.zombie_chase, PAL_ZOMBIE),
    ('rage_idle_walk', mc.rage_idle, PAL_RAGE),
    ('rage_chase_walk', mc.rage_chase, PAL_RAGE),
]


def walk_frames(fn):
    out = []
    for f in range(FRAMES):
        mc.WALK = f
        out.append(fn())
    mc.WALK = None
    return out


# ================================================================== idle -> dead (8 frames)
T = [0.0, 0.08, 0.25, 0.42, 0.58, 0.74, 0.89, 1.0]      # how far the body has fallen (f7 = final dead pose)

SPRAY = {
    1: [(21, 13, 2), (23, 18, 2), (25, 15, 1), (24, 11, 1), (26, 20, 1)],
    2: [(26, 12, 2), (25, 21, 2), (29, 16, 1), (28, 10, 1), (27, 24, 1), (23, 25, 1)],
    3: [(27, 14, 1), (26, 22, 1)],
}


def spray(s, f, c):
    for (x, y, n) in SPRAY.get(f, []):
        if n == 2:
            s.r(x, y, x + 1, y + 1, c, pool=True)
        else:
            s.p(x, y, c, pool=True)


def pool(s, pf, c, centre, rad, blobs):
    if pf <= 0:
        return
    s.e(centre[0], centre[1], max(1.0, rad[0] * pf), max(1.0, rad[1] * pf), c, pool=True)
    if pf > 0.8:
        k = (pf - 0.8) / 0.2
        for (bx, by), (rx, ry) in blobs:
            s.e(bx, by, max(1.0, rx * k), max(1.0, ry * k), c, pool=True)


def legs(s, pl, hips, ends, leg_c, boot_c, boot_rows):
    if pl <= 0:
        return
    for hy, ey in zip(hips, ends):
        ex, ey2 = lr(9, 3, pl), lr(hy, ey, pl)
        s.limb(9, hy, ex, ey2, leg_c, 3)
        if pl > 0.6:
            s.r(ex - 1, ey2 - 1, ex, ey2 + 1, boot_c)


# ---------------------------------------------------------------- main character (pistol idle -> dead)
def main_dead_frame(f, t):
    s = Sprite()
    s.dx = -1 if f == 1 else 0                                   # hit: jerked backwards
    pf = clamp((t - 0.15) / 0.85)
    pool(s, pf, 'r', (15, 17), (11, 8), [((7, 24), (3, 2)), ((26, 10), (3, 2)), ((24, 25), (2, 2))])
    spray(s, f, 'r')
    legs(s, clamp((t - 0.3) / 0.5), (14, 18), (11, 21), 'g', 'k', None)
    g = clamp((t - 0.25) / 0.45)                                 # the pistol is dropped and ends up under the body
    gx, gy = -lr(0, 7, g), lr(0, 2, g)
    mc.gun(s, [(19 + gx, 15 + gy, 24 + gx, 17 + gy, 'g'), (21 + gx, 15 + gy, 27 + gx, 16 + gy, 'l'),
               (21 + gx, 17 + gy, 23 + gx, 17 + gy, 'b')])
    cx = lr(11, 15, t)
    hx = lr(13, 23, t)

    def torso():
        s.e(cx, 16, lerp(5, 6, t), lerp(8, 4, t), 'b')

    def backpack():
        s.r(*[lr(a, b, t) for a, b in zip((5, 12, 7, 20), (9, 13, 12, 19))], 'g')
        s.r(*[lr(a, b, t) for a, b in zip((6, 13, 6, 19), (10, 14, 11, 18))], 'b')

    def arms():
        pts = (((12, 10), (14, 12)), ((16, 11), (16, 8)), ((20, 13), (19, 5)))
        top = [(lr(a[0], b[0], t), lr(a[1], b[1], t)) for a, b in pts]
        pts = (((12, 22), (14, 21)), ((16, 21), (17, 24)), ((20, 19), (20, 27)))
        bot = [(lr(a[0], b[0], t), lr(a[1], b[1], t)) for a, b in pts]
        for sh, el, hd in (top, bot):
            s.limb(sh[0], sh[1], el[0], el[1], 'b')
            s.limb(el[0], el[1], hd[0], hd[1], 'g' if t < 0.55 else 'b')

    def hands():
        s.r(lr(19, 19, t), lr(13, 4, t), lr(20, 20, t), lr(14, 5, t), 's')
        s.r(lr(19, 20, t), lr(18, 26, t), lr(20, 21, t), lr(19, 27, t), 's')

    def head():
        if t <= 0.5:
            s.head(hx, 16, 4, 's'); s.e(hx - 1, 16, 3, 3.5, 'b'); s.r(hx + 2, 14, hx + 2, 18, 'g')
            s.p(hx - 1, 16, 'g'); s.p(hx - 2, 14, 'g'); s.p(hx - 2, 18, 'g'); s.p(hx - 3, 16, 'g')
        else:
            s.head(hx, 16, 3, 'b'); s.p(hx, 16, 'g'); s.p(hx - 1, 14, 'g'); s.p(hx - 1, 18, 'g')
            s.r(hx + 3, 15, hx + 4, 17, 'g'); s.p(hx + 2, 19, 's'); s.p(hx + 3, 19, 's')

    if t <= 0.5:
        backpack(); torso()
        s.e(cx, lr(9, 13, t * 2), 3, 2, 'g'); s.e(cx, lr(23, 19, t * 2), 3, 2, 'g')     # shoulder pads sink in
        head(); arms(); hands()
    else:
        arms(); hands(); torso(); backpack(); head()
        s.r(14, 16, 16, 17, 'r')                                 # wound
    return s.finish()


# ---------------------------------------------------------------- normal zombie
def zombie_dead_frame(f, t):
    s = Sprite()
    s.dx = -1 if f == 1 else 0
    pf = clamp((t - 0.15) / 0.85)
    pool(s, pf, 'r', (15, 17), (11, 8), [((7, 24), (3, 2)), ((27, 24), (2, 2)), ((26, 9), (2, 2))])
    spray(s, f, 'r')
    legs(s, clamp((t - 0.3) / 0.5), (14, 18), (10, 23), 'm', 'd', None)
    cx = lr(14, 15, t)
    hx = lr(17, 23, t)

    def torso():
        s.e(cx, 16, lerp(5, 6, t), lerp(8, 4, t), 'd')
        s.r(*[lr(a, b, t) for a, b in zip((11, 10, 14, 12), (10, 14, 13, 16))], 'g')       # rags
        s.r(*[lr(a, b, t) for a, b in zip((12, 19, 14, 21), (15, 17, 17, 18))], 'r')       # blood stain
        if t <= 0.5:
            s.p(cx + 2, 14, 'm'); s.p(cx + 1, 13, 'm'); s.p(cx - 3, 17, 'm')
        else:
            s.p(18, 15, 'm')

    def arms():
        s.limb(14, lr(7, 12, t), lr(20, 17, t), lr(6, 5, t), 'm')
        s.limb(14, lr(25, 21, t), lr(19, 22, t), lr(27, 26, t), 'm')
        s.r(lr(20, 16, t), lr(5, 4, t), lr(20, 16, t) + 1, lr(5, 4, t) + 1, 'l')
        s.r(lr(19, 22, t), lr(27, 26, t), lr(19, 22, t) + 1, lr(27, 26, t) + 1, 'l')

    def head():
        s.head(hx, 16, 4 if t <= 0.5 else 3, 'l')
        spots = ([(1, -2, 'r'), (2, -2, 'r'), (-2, 2, 'm'), (-1, 2, 'm'), (-2, -2, 'm')] if t <= 0.5
                 else [(1, -1, 'r'), (2, 0, 'r'), (-1, 2, 'm')])
        for dx, dy, c in spots:
            s.p(hx + dx, 16 + dy, c)

    if t <= 0.5:
        torso(); arms(); head()
    else:
        arms(); torso(); head()
    return s.finish()


# ---------------------------------------------------------------- rage zombie
def rage_dead_frame(f, t):
    s = Sprite()
    s.dx = -1 if f == 1 else 0
    pf = clamp((t - 0.15) / 0.85)
    pool(s, pf, 'p', (15, 17), (12, 9), [((6, 25), (3, 2)), ((27, 25), (2, 2)), ((27, 8), (3, 2))])
    spray(s, f, 'l')
    legs(s, clamp((t - 0.3) / 0.5), (14, 18), (10, 22), 'd', 'k', None)
    cx = lr(14, 15, t)
    hx = lr(17, 24, t)

    def torso():
        s.e(cx, 16, lerp(6, 7, t), lerp(10, 5, t), 'd')
        if t <= 0.5:
            s.e(cx, lr(7, 12, t * 2), 3, 2, 'm'); s.e(cx, lr(25, 20, t * 2), 3, 2, 'm')
            if t <= 0.4:
                for y in (12, 14, 16, 18, 20):
                    s.r(cx - 5, y, cx - 2, y, 'm'); s.p(cx - 6, y, 'l')
                s.p(cx - 1, 12, 'y'); s.p(cx, 13, 'y'); s.p(cx - 1, 19, 'y'); s.p(cx, 20, 'y')
        else:
            for x in (8, 10, 12):
                s.p(x, 15, 'm'); s.p(x, 17, 'm')
            s.p(13, 15, 'y'); s.p(14, 17, 'y')

    def arms():
        shy = lr(5, 11, t)
        s.limb(14, shy, lr(21, 19, t), lr(5, 4, t), 'm', 3)
        ox, oy = lr(22, 19, t), lr(4, 3, t)
        s.r(ox, oy, ox + 2, oy + (2 if t < 0.8 else 1), 'l')
        s.p(ox + 3, oy, 'g')
        if t < 0.7:
            s.p(ox + 3, oy + 2, 'g')
        shy = lr(27, 22, t)
        s.limb(14, shy, lr(20, 21, t), lr(27, 27, t), 'm', 3)
        ox, oy = lr(21, 21, t), lr(26, 27, t)
        s.r(ox, oy, ox + 2, oy + 2 if t < 0.8 else oy + 1, 'l')
        if t < 0.7:
            s.p(ox + 3, oy, 'g'); s.p(ox + 3, oy + 2, 'g')
        else:
            s.p(ox + 3, oy + 1, 'g')

    def head():
        if t <= 0.5:
            s.head(hx, 16, 4, 'm')
            s.p(hx + 2, 14, 'y'); s.p(hx + 2, 18, 'y')
            s.r(hx - 1, 11, hx, 12, 'l'); s.r(hx - 1, 20, hx, 21, 'l')
            s.p(hx - 2, 15, 'd'); s.p(hx - 2, 17, 'd')
        else:
            s.head(hx, 16, 3, 'm')
            s.p(hx - 1, 13, 'l'); s.p(hx - 1, 19, 'l'); s.p(hx + 2, 15, 'y')

    if t <= 0.5:
        torso(); arms(); head()
    else:
        arms(); torso(); head()
    return s.finish()


DEADS = [
    ('main_dead_anim', mc.main_pistol, mc.main_dead, main_dead_frame, PAL_MAIN),
    ('zombie_dead_anim', mc.zombie_idle, mc.zombie_dead, zombie_dead_frame, PAL_ZOMBIE),
    ('rage_dead_anim', mc.rage_idle, mc.rage_dead, rage_dead_frame, PAL_RAGE),
]


def dead_frames(first, last, morph):
    mc.WALK = None
    fr = [first()]
    for f in range(1, FRAMES - 1):
        fr.append(morph(f, T[f]))
    fr.append(last())
    return fr


# ================================================================== output
def write_strip(path, frames, pal_map):
    sw = W * len(frames)
    rows = [''.join(fr[y] for fr in frames) for y in range(H)]
    used = sorted({c for r in rows for c in r}, key=lambda c: (c != '.', c))
    assert used[0] == '.'
    idx = {c: i for i, c in enumerate(used)}
    pal = [pal_map[c] for c in used]
    off = 14 + 40 + 4 * len(pal)
    hdr = b'BM' + struct.pack('<IHHI', off + sw * H, 0, 0, off)
    info = struct.pack('<IiiHHIIiiII', 40, sw, H, 1, 8, 0, sw * H, 2835, 2835, len(pal), len(pal))
    pal_b = b''.join(bytes((b, g, r, 0)) for r, g, b in pal)
    body = b''.join(bytes(idx[c] for c in rows[y]) for y in range(H - 1, -1, -1))
    with open(path, 'wb') as fh:
        fh.write(hdr + info + pal_b + body)
    return len(pal)


ALL = {}
for name, fn, pal in WALKS:
    ALL[name] = (walk_frames(fn), pal)
for name, first, last, morph, pal in DEADS:
    ALL[name] = (dead_frames(first, last, morph), pal)

for name, (frames, pal) in ALL.items():
    assert len(frames) == FRAMES
    assert len({tuple(fr) for fr in frames}) == FRAMES, f'{name}: duplicate frames'
    for fr in frames:
        assert len(fr) == H and all(len(r) == W for r in fr)
        for r in fr:
            for c in r:
                assert c == '.' or pal[c] != KEY
    n = write_strip(f'{OUT}/{name}.bmp', frames, pal)
    assert n <= 8, (name, n)
    print(f'{name}.bmp: {W * FRAMES}x{H}, {FRAMES} frames, {n} palette entries incl. transparent key')

# ---- previews
BG = (150, 158, 168)
S, GAP = 3, 4
names = list(ALL)
cw = FRAMES * W + (FRAMES + 1) * GAP
ch = len(names) * H + (len(names) + 1) * GAP
canvas = [[BG] * cw for _ in range(ch)]
for ri, nm in enumerate(names):
    frames, pal = ALL[nm]
    for fi, fr in enumerate(frames):
        ox, oy = GAP + fi * (W + GAP), GAP + ri * (H + GAP)
        for y in range(H):
            for x in range(W):
                c = fr[y][x]
                if c != '.':
                    canvas[oy + y][ox + x] = pal[c]
big = []
for row in canvas:
    wide = [p for p in row for _ in range(S)]
    big.extend([wide] * S)
write_png(f'{OUT}/preview_all.png', big)

try:
    from PIL import Image

    def tile(frame, pal, scale):
        im = Image.new('RGB', (W * scale, H * scale), BG)
        im.putdata([BG if c == '.' else pal[c] for r in frame for c in r for _ in range(1)] and
                   [BG] * 0 or [BG] * (W * scale * H * scale))
        px = im.load()
        for y in range(H):
            for x in range(W):
                c = frame[y][x]
                if c != '.':
                    for dy in range(scale):
                        for dx in range(scale):
                            px[x * scale + dx, y * scale + dy] = pal[c]
        return im

    def sheet_gif(path, group, cols, scale, last_hold=0):
        gap = 8
        rows_n = math.ceil(len(group) / cols)
        tw = W * scale
        ims = []
        for f in range(FRAMES):
            im = Image.new('RGB', (cols * (tw + gap) + gap, rows_n * (tw + gap) + gap), (40, 44, 48))
            for i, nm in enumerate(group):
                frames, pal = ALL[nm]
                im.paste(tile(frames[f], pal, scale), (gap + (i % cols) * (tw + gap), gap + (i // cols) * (tw + gap)))
            ims.append(im)
        dur = [100] * FRAMES
        if last_hold:
            dur[-1] = last_hold
        ims[0].save(path, save_all=True, append_images=ims[1:], duration=dur, loop=0)

    sheet_gif(f'{OUT}/walk_preview.gif', [n for n, *_ in WALKS], 4, 4)
    sheet_gif(f'{OUT}/dead_preview.gif', [n for n, *_ in DEADS], 3, 5, last_hold=900)
    print('gifs written')
except ImportError:
    print('Pillow not available, no gifs')
print('ok')
