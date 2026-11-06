import math, os, random, struct, zipfile, zlib

W, H = 640, 320
FRAMES = 24
COLS, ROWS_N = 4, 6               # sprite sheet: 4 x 6 frames = 2560 x 1920
KEY = (255, 255, 255)
PAL = {
    '.': KEY,
    'D': (60, 0, 8),
    'M': (120, 8, 18),
    'B': (188, 22, 30),
    'H': (232, 92, 84),
}
CX0, CX1, CY0, CY1 = 110, 530, 50, 270   # screen centre: static blood stays out of it
SIDE_MAX, TOP_MAX, BOTTOM_KEEP = 58, 22, 0.30

# heartbeat pulse: still one lub-dub every 8 frames (same real-time speed), 3 beats per 24-frame loop
BEAT_8 = [0.0, 0.60, 1.0, 0.40, 0.10, 0.70, 0.30, 0.0]
BEAT = BEAT_8 * 3
PULSE_AMP = 0.22


def smooth(t):
    return t * t * (3 - 2 * t)


def profile(n, seed, s1=16, s2=5):
    rng = random.Random(seed)

    def knots(step):
        return [rng.random() for _ in range(n // step + 3)]

    k1, k2 = knots(s1), knots(s2)

    def at(k, i, step):
        a = i / step
        j = int(a)
        t = smooth(a - j)
        return k[j] * (1 - t) + k[j + 1] * t

    return [0.6 * at(k1, i, s1) + 0.4 * at(k2, i, s2) for i in range(n)]


NL, NR = profile(H, 11), profile(H, 12)
NT = profile(W, 13, 24, 7)


def side_t(noise, y):
    f = smooth(max(0.0, (y - 0.40 * H) / (0.60 * H)))
    taper = 1 - (1 - BOTTOM_KEEP) * f
    top_boost = 1 + 0.5 * max(0.0, 1 - y / 60)
    return SIDE_MAX * (0.4 + 0.6 * noise[y]) * taper * top_boost


def top_t(x):
    d = min(x, W - 1 - x)
    return TOP_MAX * (0.4 + 0.6 * NT[x]) * (1 + 0.4 * max(0.0, 1 - d / 90))


BASE_L = [side_t(NL, y) for y in range(H)]
BASE_R = [side_t(NR, y) for y in range(H)]
BASE_T = [top_t(x) for x in range(W)]


def put(g, x, y, c):
    if 0 <= x < W and 0 <= y < H:
        g[y][x] = c


def disc(g, cx, cy, r, fill='B'):
    pts = {(x, y) for y in range(cy - r, cy + r + 1)
           for x in range(cx - r, cx + r + 1)
           if (x - cx) ** 2 + (y - cy) ** 2 <= r * r + r // 2}
    for (x, y) in pts:
        edge = any((x + dx, y + dy) not in pts for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        put(g, x, y, 'M' if edge else fill)
    if r >= 3:
        put(g, cx - r // 2, cy - r // 2, 'H')


def clear_ok(x, y, pad=0):
    return not (CX0 - pad <= x <= CX1 + pad and CY0 - pad <= y <= CY1 + pad)


# ------------------------------------------------------------------ layout
rng = random.Random(33)
drips = []                         # [x, width, max length, is_centre, phase]


def add_drips(xs, lmin, lmax, centre):
    for x in xs:
        w = rng.choice((3, 4, 4, 5))
        drips.append([x, w, rng.randrange(lmin, lmax), centre, None])


# 5 on each side, spread in from the corners, plus 6 spread across the middle of the top edge
add_drips([10 + i * 22 + rng.randrange(-1, 2) for i in range(5)], 30, 111, False)        # left  (x < 110)
add_drips([CX1 + 8 + i * 22 + rng.randrange(-1, 2) for i in range(5)], 30, 111, False)   # right (x > 530)
add_drips([x + rng.randrange(-6, 7) for x in (170, 228, 286, 354, 412, 470)], 16, 45, True)     # centre
assert len(drips) == 16
for d in drips:
    if not d[3]:
        assert d[0] + d[1] < CX0 or d[0] > CX1

# every drip gets a different phase (5 is coprime with 16), so the stream is steady and never in sync
drips.sort(key=lambda d: d[0])
for i, d in enumerate(drips):
    d[4] = (((i * 5) % 16) * 3) // 2        # same stagger pattern as before, stretched to 24 frames

# static droplets in the margins (more than before), away from the centre and bottom
splats = []
tries = 0
while len(splats) < 48 and tries < 8000:
    tries += 1
    r = rng.choice((1, 2, 2, 2, 3, 3, 4, 5))
    cx, cy = rng.randrange(r + 2, W - r - 2), rng.randrange(r + 2, H - r - 2)
    if not clear_ok(cx, cy, r + 3) or cy > H - 70:
        continue
    splats.append((cx, cy, r))


# ------------------------------------------------------------------ drip lifecycle
# t = (frame + phase) % 24
#   t 0..17  : a bulb forms on the top edge and stretches down SLOWLY (18 frames), swelling into a big drop
#   t 18     : it pinches off: stump snaps back (30% -> 10% -> gone), the big droplet leaves
#   t 18..22 : the droplet FALLS FAST, accelerating all the way to the bottom of the screen
#   t 23, 0  : the droplet is off-screen; then a brand-new drip starts at the top
GROW = 18
FALL = 6                                  # fall steps; the last one is already below the screen


def fall_positions(Lmax):
    y0 = Lmax + 1
    dist = H + 14 - y0                    # always ends below the bottom edge
    return [y0 + round(dist * ((k + 1) / FALL) ** 1.7) for k in range(FALL)]


def draw_drip(g, d, f):
    x, w, Lmax, centre, ph = d
    t = (f + ph) % FRAMES
    r0 = w // 2 + 1
    rd = r0 + 2                                  # big droplet
    cols = ['D'] + ['B'] * (w - 2) + ['M']
    cx = x + w // 2

    def stem(length, r):
        for y in range(length):
            for i, c in enumerate(cols):
                put(g, x + i, y, c)
        for i in (0, w - 1):
            put(g, x + i, length - 1, '.')
        disc(g, cx, length + 1, r)

    if t < GROW:
        length = max(2, round(Lmax * ((t + 1) / GROW) ** 1.6))
        r = r0 if t < 9 else r0 + 1 if t < 14 else rd
        stem(length, r)
        return
    if t == GROW:
        stem(max(2, round(Lmax * 0.30)), r0)
    elif t == GROW + 1:
        stem(max(2, round(Lmax * 0.10)), r0)
    k = t - GROW                                 # 0..5
    y = fall_positions(Lmax)[k]
    if y - rd >= H:
        return                                   # fully off the bottom of the screen
    tail = (0, 2, 4, 6, 7, 7)[k]
    for j in range(1, tail + 1):                 # longer motion streak: it is falling fast now
        put(g, cx - 1, y - rd - j, 'M')
        put(g, cx, y - rd - j, 'B')
    disc(g, cx, y, rd)


# every droplet must leave the screen before the loop restarts
for d in drips:
    rd = d[1] // 2 + 3
    assert fall_positions(d[2])[-1] - rd >= H, d
    assert GROW + FALL <= FRAMES

def render(f, centre_drips=True):
    s = 1 + PULSE_AMP * BEAT[f]
    tL = [round(v * s) for v in BASE_L]
    tR = [round(v * s) for v in BASE_R]
    tT = [round(v * s) for v in BASE_T]
    g = [['.'] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            best = None
            for dist, t in ((x, tL[y]), (W - 1 - x, tR[y]), (y, tT[x])):   # no bottom edge
                if dist < t:
                    q = dist / t
                    best = q if best is None else min(best, q)
            if best is None:
                continue
            if best < 0.30:
                c = 'D'
            elif best < 0.65:
                c = 'M'
            else:
                c = 'B'
                if best > 0.9 and ((x // 2) * 31 + (y // 2) * 17) % 9 == 0:
                    c = 'H'
            g[y][x] = c
    for (cx, cy, r) in splats:
        disc(g, cx, cy, r)
    for d in drips:
        if not d[3]:
            draw_drip(g, d, f)
    base = [''.join(r) for r in g]                       # everything except centre drips
    if centre_drips:
        for d in drips:
            if d[3]:
                draw_drip(g, d, f)
    return base, [''.join(r) for r in g]


def write_bmp(path, rows, chars):
    h, w = len(rows), len(rows[0])
    assert w % 4 == 0
    idx = {c: i for i, c in enumerate(chars)}
    pal = [PAL[c] for c in chars]
    off = 14 + 40 + 4 * len(pal)
    hdr = b'BM' + struct.pack('<IHHI', off + w * h, 0, 0, off)
    info = struct.pack('<IiiHHIIiiII', 40, w, h, 1, 8, 0, w * h, 2835, 2835, len(pal), len(pal))
    pal_b = b''.join(bytes((b, gg, r, 0)) for r, gg, b in pal)
    body = b''.join(bytes(idx[c] for c in rows[y]) for y in range(h - 1, -1, -1))
    with open(path, 'wb') as fh:
        fh.write(hdr + info + pal_b + body)
    return len(pal)


def write_png(path, px):
    h, w = len(px), len(px[0])
    raw = b''.join(b'\x00' + bytes(v for p in row for v in p) for row in px)

    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)

    with open(path, 'wb') as fh:
        fh.write(b'\x89PNG\r\n\x1a\n'
                 + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                 + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


OUT = '/home/claude/bleed_anim4'
os.makedirs(f'{OUT}/frames', exist_ok=True)
CHARS = ['.', 'B', 'D', 'H', 'M']
bases, frames = zip(*[render(f) for f in range(FRAMES)])

# ---- checks on every frame ----
assert len({tuple(fr) for fr in frames}) == FRAMES, 'duplicate frames'
max_centre = 0
for f, (b, rows) in enumerate(zip(bases, frames)):
    assert len(rows) == H and all(len(r) == W for r in rows)
    for y, r in enumerate(b):                    # edges, splats, side drips: centre stays empty
        for x, c in enumerate(r):
            if c != '.':
                assert clear_ok(x, y), f'frame {f}: static/side blood in centre at {x},{y}'
    assert sum(1 for r in b[H - 40:] for c in r[CX0:CX1 + 1] if c != '.') == 0, f'frame {f}: bottom edge'
    for r in rows:
        for c in r:
            assert c == '.' or PAL[c] != KEY
    n_in = sum(1 for d in drips if d[3] and any(
        rows[y][x] != '.' and not clear_ok(x, y)
        for y in range(CY0, CY1 + 1) for x in range(d[0] - 1, d[0] + d[1] + 1)))
    max_centre = max(max_centre, n_in)
    assert write_bmp(f'{OUT}/frames/bleed_{f:02d}.bmp', rows, CHARS) <= 8

sheet = []
for gr in range(ROWS_N):
    for y in range(H):
        sheet.append(''.join(frames[gr * COLS + gc][y] for gc in range(COLS)))
write_bmp(f'{OUT}/bleed_anim_sheet.bmp', sheet, CHARS)
with zipfile.ZipFile(f'{OUT}/bleed_frames.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    for f in range(FRAMES):
        z.write(f'{OUT}/frames/bleed_{f:02d}.bmp', f'bleed_{f:02d}.bmp')

# ---- previews ----
BG = (96, 108, 100)
GREY = (40, 44, 48)
half = []
for gr in range(ROWS_N):
    for y in range(0, H, 2):
        line = []
        for gc in range(COLS):
            fr = frames[gr * COLS + gc]
            line.extend(BG if c == '.' else PAL[c] for c in fr[y][::2])
            line.extend([GREY] * 4)
        half.append(line)
    half.extend([[GREY] * len(half[0])] * 4)
write_png(f'{OUT}/contact_sheet.png', half)

# a few frames at full size, to judge the drip shapes
pick = [6, 12, 19, 21]
full = []
for y in range(H):
    line = []
    for f in pick[:2]:
        line.extend(BG if c == '.' else PAL[c] for c in frames[f][y])
        line.extend([GREY] * 6)
    full.append(line)
full.extend([[GREY] * len(full[0])] * 6)
for y in range(H):
    line = []
    for f in pick[2:]:
        line.extend(BG if c == '.' else PAL[c] for c in frames[f][y])
        line.extend([GREY] * 6)
    full.append(line)
write_png(f'{OUT}/frames_6_12_19_21.png', full)

try:
    from PIL import Image
    ims = []
    for rows in frames:
        im = Image.new('RGB', (W, H))
        im.putdata([BG if c == '.' else PAL[c] for r in rows for c in r])
        ims.append(im)
    ims[0].save(f'{OUT}/bleed_preview.gif', save_all=True, append_images=ims[1:], duration=80, loop=0)
    print('gif written')
except ImportError:
    print('Pillow not available, no gif')

print(f'{FRAMES} frames {W}x{H}; sheet {W * COLS}x{H * ROWS_N}; palette {len(CHARS)} entries incl. key')
print(f'drips: {len(drips)} total ({sum(1 for d in drips if not d[3])} on the sides + {sum(1 for d in drips if d[3])} across the middle); '
      f'{len(splats)} static droplets; at most {max_centre} centre drips visible in the centre at once')
print('all frames distinct; edges/splats/side drips never enter the centre; no bottom edge; key colour unused')
