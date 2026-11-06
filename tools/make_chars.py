import math, os, struct, zlib

W = H = 32
KEY = (255, 255, 255)

PAL_MAIN = {
    '.': KEY,
    'k': (8, 8, 10),        # outline
    'b': (32, 32, 36),      # black clothing / hair
    'g': (70, 72, 80),      # dark grey
    'l': (128, 132, 142),   # light grey (metal highlights)
    's': (208, 162, 130),   # skin
    'r': (112, 16, 22),     # blood
}
PAL_ZOMBIE = {
    '.': KEY,
    'k': (6, 14, 8),
    'd': (30, 64, 38),      # dark green
    'm': (56, 104, 60),     # mid green
    'l': (112, 152, 92),    # pale green skin
    'g': (68, 70, 76),      # grey rags
    'r': (120, 16, 22),     # blood
}
PAL_RAGE = {
    '.': KEY,
    'k': (16, 2, 4),
    'd': (100, 12, 18),     # dark red
    'm': (152, 22, 28),     # mid red
    'l': (208, 70, 56),     # bright red highlights
    'g': (74, 74, 80),      # grey claws
    'y': (244, 196, 60),    # glowing cracks
    'p': (66, 6, 12),       # blood pool
}


class Sprite:
    def __init__(self):
        self.body = {}
        self.pool = {}
        self.dx = 0
        self.dy = 0

    def _t(self, pool):
        return self.pool if pool else self.body

    def p(self, x, y, c, pool=False):
        self._t(pool)[(x, y)] = c

    def r(self, x0, y0, x1, y1, c, pool=False):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self._t(pool)[(x, y)] = c

    def e(self, cx, cy, rx, ry, c, pool=False):
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            for x in range(int(cx - rx - 1), int(cx + rx + 2)):
                if ((x - cx) / (rx + 0.4)) ** 2 + ((y - cy) / (ry + 0.4)) ** 2 <= 1:
                    self._t(pool)[(x, y)] = c

    def l(self, x0, y0, x1, y1, c, t=2, pool=False):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        lo, hi = -(t // 2), t - 1 - (t // 2)
        for i in range(n + 1):
            x = round(x0 + (x1 - x0) * i / n)
            y = round(y0 + (y1 - y0) * i / n)
            for dy in range(lo, hi + 1):
                for dx in range(lo, hi + 1):
                    self._t(pool)[(x + dx, y + dy)] = c

    def limb(self, x0, y0, x1, y1, c, t=2):
        """thick line with a 1px dark border so it reads against the torso"""
        self.l(x0, y0, x1, y1, 'k', t + 2)
        self.l(x0, y0, x1, y1, c, t)

    def head(self, cx, cy, rad, c):
        self.e(cx, cy, rad + 1, rad + 1, 'k')
        self.e(cx, cy, rad, rad, c)

    def finish(self):
        body = {(x + self.dx, y + self.dy): c for (x, y), c in self.body.items()}
        ring = {}
        for (x, y) in body:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                q = (x + dx, y + dy)
                if q not in body:
                    ring[q] = 'k'
        out = {}
        out.update(self.pool)
        out.update(ring)
        out.update(body)
        grid = [['.'] * W for _ in range(H)]
        for (x, y), c in out.items():
            assert 0 <= x < W and 0 <= y < H, (x, y)
            grid[y][x] = c
        return [''.join(r) for r in grid]


# ------------------------------------------------------------------ walk-cycle support
WALK = None        # None = normal static pose; 0..7 = walk-cycle frame (set by make_anims.py)


def wsin():
    return 0.0 if WALK is None else math.sin(2 * math.pi * WALK / 8)


def wcos():
    return 0.0 if WALK is None else math.cos(2 * math.pi * WALK / 8)


def walk_setup(s):
    """lateral sway of the whole body while walking"""
    if WALK is not None:
        s.dy = round(wcos())


def walk_legs(s, hip_x, off, amp, leg, foot, hip_y=16):
    """two legs under the body (drawn first so the torso covers them); feet step in and out in front and behind"""
    if WALK is None:
        return
    sn = wsin()
    for sign, y in ((1, hip_y - off), (-1, hip_y + off)):
        fx = hip_x + round(sign * amp * sn)
        s.l(hip_x, y, fx, y, leg, 3)
        s.r(fx - 1, y - 1, fx + 1, y + 1, foot)


# ------------------------------------------------------------------ main character (black)
def main_base(s):
    walk_setup(s)
    walk_legs(s, 11, 6, 9, 'g', 'l')
    s.r(5, 12, 7, 20, 'g')                  # backpack
    s.r(6, 13, 6, 19, 'b')
    s.e(11, 16, 5, 8, 'b')                  # torso (shifted back so the weapon sticks out further)
    s.e(11, 9, 3, 2, 'g')                   # shoulder pads
    s.e(11, 23, 3, 2, 'g')
    s.head(13, 16, 4, 's')                  # head (skin shows at the front = face)
    s.e(12, 16, 3, 3.5, 'b')                # cap crown
    s.r(15, 14, 15, 18, 'g')                # cap brim edge, face visible in front of it
    s.p(12, 16, 'g'); s.p(11, 14, 'g'); s.p(11, 18, 'g'); s.p(10, 16, 'g')   # cap button + seams


def arms(s, top, bottom):
    """top / bottom = [shoulder, elbow, hand]; black upper arm, grey forearm"""
    for sh, el, hd in (top, bottom):
        s.limb(sh[0], sh[1], el[0], el[1], 'b')
        s.limb(el[0], el[1], hd[0], hd[1], 'g')


def hand(s, x, y):
    s.r(x, y, x + 1, y + 1, 's')


def gun(s, parts):
    """dark border around every part first, then the coloured fills, so the gun separates from the body"""
    for (x0, y0, x1, y1, c) in parts:
        if c != 'k':
            s.r(x0 - 1, y0 - 1, x1 + 1, y1 + 1, 'k')
    for (x0, y0, x1, y1, c) in parts:
        s.r(x0, y0, x1, y1, c)


def main_pistol():
    s = Sprite(); main_base(s)
    arms(s, [(12, 10), (16, 11), (20, 13)], [(12, 22), (16, 21), (20, 19)])
    gun(s, [(19, 15, 24, 17, 'g'), (21, 15, 27, 16, 'l'), (21, 17, 23, 17, 'b')])
    hand(s, 19, 13); hand(s, 19, 18)
    return s.finish()


def main_rifle():
    s = Sprite(); main_base(s)
    arms(s, [(12, 10), (19, 10), (26, 12)], [(12, 22), (17, 21), (22, 19)])
    gun(s, [(19, 15, 21, 17, 'g'), (22, 15, 26, 17, 'l'), (23, 13, 24, 19, 'l'),
            (27, 15, 28, 17, 'g'), (27, 16, 29, 16, 'l')])
    s.r(22, 16, 26, 16, 'g'); s.r(23, 14, 24, 18, 'g')               # receiver line, optic body
    hand(s, 25, 13); hand(s, 21, 18)
    return s.finish()


def main_shotgun():
    s = Sprite(); main_base(s)
    arms(s, [(12, 10), (17, 11), (22, 13)], [(12, 22), (19, 22), (25, 20)])
    gun(s, [(19, 15, 21, 17, 'g'), (22, 15, 25, 17, 'l'), (26, 15, 29, 15, 'l'),
            (26, 17, 28, 17, 'g'), (25, 18, 28, 19, 'b')])
    s.r(26, 16, 29, 16, 'k'); s.r(26, 18, 27, 18, 'g')               # barrel gap, pump highlight
    hand(s, 22, 13); hand(s, 25, 19)
    return s.finish()


def main_smg():
    s = Sprite(); main_base(s)
    arms(s, [(12, 10), (16, 11), (20, 13)], [(12, 22), (17, 21), (24, 20)])
    gun(s, [(19, 15, 26, 17, 'g'), (24, 18, 25, 18, 'b'), (27, 16, 28, 16, 'g')])
    s.r(20, 16, 26, 16, 'l'); s.r(20, 15, 22, 15, 'l')
    hand(s, 19, 13); hand(s, 24, 19)
    return s.finish()


def prone_legs(s, leg_c, boot_c, spread=(11, 21)):
    s.limb(9, 14, 3, spread[0], leg_c, 3)
    s.limb(9, 18, 3, spread[1], leg_c, 3)
    s.r(2, spread[0] - 1, 3, spread[0] + 1, boot_c)
    s.r(2, spread[1] - 1, 3, spread[1] + 1, boot_c)


def main_dead():
    s = Sprite()
    s.e(15, 17, 11, 8, 'r', pool=True)                      # blood pool
    s.e(7, 24, 3, 2, 'r', pool=True); s.e(26, 10, 3, 2, 'r', pool=True); s.e(24, 25, 2, 2, 'r', pool=True)
    prone_legs(s, 'g', 'k')
    s.limb(14, 12, 19, 5, 'b'); s.limb(14, 21, 20, 27, 'b')  # arms flung out
    s.r(19, 4, 20, 5, 's'); s.r(20, 26, 21, 27, 's')
    s.e(15, 16, 6, 4, 'b')                                  # torso (prone)
    s.r(9, 13, 12, 19, 'g'); s.r(10, 14, 11, 18, 'b')       # backpack on top
    s.head(23, 16, 3, 'b')
    s.p(23, 16, 'g'); s.p(22, 14, 'g'); s.p(22, 18, 'g')    # cap button + seams
    s.r(26, 15, 27, 17, 'g')                                # brim sticking out
    s.p(25, 19, 's'); s.p(26, 19, 's')                      # cheek / chin turned to the side
    s.r(14, 16, 16, 17, 'r')                                # wound
    return s.finish()


# ------------------------------------------------------------------ normal zombie (dark green)
def zombie_torso(s, cx=14):
    walk_legs(s, cx, 5, 8, 'd', 'g')
    s.e(cx, 16, 5, 8, 'd')
    s.r(cx - 3, 10, cx, 12, 'g')            # torn rags
    s.r(cx - 2, 19, cx, 21, 'r')            # blood stain
    s.p(cx + 2, 14, 'm'); s.p(cx + 1, 13, 'm'); s.p(cx - 3, 17, 'm')


def zombie_idle():
    s = Sprite(); walk_setup(s)
    zombie_torso(s)
    a = round(3 * wsin())                                       # arm swing while walking (0 when static)
    s.limb(14, 7, 20 - a, 6, 'm'); s.limb(14, 25, 19 + a, 27, 'm')      # arms hanging at the sides
    s.r(20 - a, 5, 21 - a, 6, 'l'); s.r(19 + a, 27, 20 + a, 28, 'l')
    s.head(17, 16, 4, 'l')
    s.p(18, 14, 'r'); s.p(19, 14, 'r'); s.p(15, 18, 'm'); s.p(16, 18, 'm'); s.p(15, 14, 'm')
    return s.finish()


def zombie_chase():
    s = Sprite(); walk_setup(s)
    zombie_torso(s, 15)
    a = round(1 * wsin())                                       # hands reach in and out while walking
    s.limb(15, 9, 26 + a, 12, 'm'); s.limb(15, 23, 26 - a, 20, 'm')     # arms raised forward
    s.r(26 + a, 10, 28 + a, 12, 'l'); s.r(26 - a, 20, 28 - a, 22, 'l')
    s.p(29 + a, 10, 'l'); s.p(29 + a, 12, 'l'); s.p(29 - a, 20, 'l'); s.p(29 - a, 22, 'l')   # fingers
    s.head(19, 16, 4, 'l')
    s.p(20, 14, 'r'); s.p(21, 14, 'r'); s.p(17, 18, 'm'); s.p(18, 18, 'm'); s.p(17, 14, 'm')
    return s.finish()


def zombie_dead():
    s = Sprite()
    s.e(15, 17, 11, 8, 'r', pool=True)
    s.e(7, 24, 3, 2, 'r', pool=True); s.e(27, 24, 2, 2, 'r', pool=True); s.e(26, 9, 2, 2, 'r', pool=True)
    prone_legs(s, 'm', 'd', spread=(10, 23))
    s.limb(14, 12, 17, 5, 'm'); s.limb(14, 21, 22, 26, 'm')
    s.r(16, 4, 17, 5, 'l'); s.r(22, 26, 23, 27, 'l')
    s.e(15, 16, 6, 4, 'd')
    s.r(10, 14, 13, 16, 'g'); s.r(15, 17, 17, 18, 'r'); s.p(18, 15, 'm')
    s.head(23, 16, 3, 'l')
    s.p(24, 15, 'r'); s.p(25, 16, 'r'); s.p(22, 18, 'm')
    return s.finish()


# ------------------------------------------------------------------ rage zombie (dark red)
def rage_torso(s, cx=14):
    walk_legs(s, cx, 6, 8, 'd', 'g')
    s.e(cx, 16, 6, 10, 'd')
    s.e(cx, 7, 3, 2, 'm'); s.e(cx, 25, 3, 2, 'm')           # heavy shoulders
    for i, y in enumerate((12, 14, 16, 18, 20)):            # bony spine ridge
        s.r(cx - 5, y, cx - 2, y, 'm')
        s.p(cx - 6, y, 'l')
    s.p(cx - 1, 12, 'y'); s.p(cx, 13, 'y'); s.p(cx - 1, 19, 'y'); s.p(cx, 20, 'y')   # glowing cracks


def rage_head(s, cx):
    s.head(cx, 16, 4, 'm')
    s.p(cx + 2, 14, 'y'); s.p(cx + 2, 18, 'y')              # glowing eyes at the front
    s.r(cx - 1, 11, cx, 12, 'l'); s.r(cx - 1, 20, cx, 21, 'l')   # horns
    s.p(cx - 2, 15, 'd'); s.p(cx - 2, 17, 'd')


def rage_idle():
    s = Sprite(); walk_setup(s)
    rage_torso(s)
    a = round(3 * wsin())
    s.limb(14, 5, 21 - a, 5, 'm', 3); s.limb(14, 27, 20 + a, 27, 'm', 3)
    s.r(22 - a, 4, 24 - a, 6, 'l'); s.r(21 + a, 26, 23 + a, 28, 'l')
    s.p(25 - a, 4, 'g'); s.p(25 - a, 6, 'g'); s.p(24 + a, 26, 'g'); s.p(24 + a, 28, 'g')
    rage_head(s, 17)
    return s.finish()


def rage_chase():
    s = Sprite(); walk_setup(s)
    rage_torso(s, 14)
    a = round(1 * wsin())
    s.limb(14, 8, 25 + a, 11, 'm', 3); s.limb(14, 24, 25 - a, 21, 'm', 3)
    s.r(25 + a, 10, 27 + a, 12, 'l'); s.r(25 - a, 20, 27 - a, 22, 'l')
    for yy in (9, 11, 13):
        s.p(28 + a, yy, 'g'); s.p(29 + a, yy, 'g')              # claws
    for yy in (19, 21, 23):
        s.p(28 - a, yy, 'g'); s.p(29 - a, yy, 'g')
    rage_head(s, 18)
    return s.finish()


def rage_dead():
    s = Sprite()
    s.e(15, 17, 12, 9, 'p', pool=True)
    s.e(6, 25, 3, 2, 'p', pool=True); s.e(27, 25, 2, 2, 'p', pool=True); s.e(27, 8, 3, 2, 'p', pool=True)
    prone_legs(s, 'd', 'k', spread=(10, 22))
    s.limb(14, 11, 19, 4, 'm', 3); s.limb(14, 22, 21, 27, 'm', 3)
    s.r(19, 3, 21, 4, 'l'); s.r(21, 27, 23, 28, 'l')
    s.p(22, 3, 'g'); s.p(24, 28, 'g')
    s.e(15, 16, 7, 5, 'd')
    for x in (8, 10, 12):                                   # spine ridge
        s.p(x, 15, 'm'); s.p(x, 17, 'm')
    s.p(13, 15, 'y'); s.p(14, 17, 'y')
    s.head(24, 16, 3, 'm')
    s.p(23, 13, 'l'); s.p(23, 19, 'l'); s.p(26, 15, 'y')
    return s.finish()


SPRITES = {
    'main_pistol': (main_pistol, PAL_MAIN),
    'main_rifle': (main_rifle, PAL_MAIN),
    'main_shotgun': (main_shotgun, PAL_MAIN),
    'main_smg': (main_smg, PAL_MAIN),
    'main_dead': (main_dead, PAL_MAIN),
    'zombie_idle': (zombie_idle, PAL_ZOMBIE),
    'zombie_chase': (zombie_chase, PAL_ZOMBIE),
    'zombie_dead': (zombie_dead, PAL_ZOMBIE),
    'rage_idle': (rage_idle, PAL_RAGE),
    'rage_chase': (rage_chase, PAL_RAGE),
    'rage_dead': (rage_dead, PAL_RAGE),
}


def write_bmp(path, rows, pal_map):
    used = sorted({c for r in rows for c in r}, key=lambda c: (c != '.', c))
    assert used[0] == '.'
    idx = {c: i for i, c in enumerate(used)}
    pal = [pal_map[c] for c in used]
    off = 14 + 40 + 4 * len(pal)
    hdr = b'BM' + struct.pack('<IHHI', off + W * H, 0, 0, off)
    info = struct.pack('<IiiHHIIiiII', 40, W, H, 1, 8, 0, W * H, 2835, 2835, len(pal), len(pal))
    pal_b = b''.join(bytes((b, g, r, 0)) for r, g, b in pal)
    body = b''.join(bytes(idx[c] for c in rows[y]) for y in range(H - 1, -1, -1))
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


OUT = '/home/claude/chars'
os.makedirs(OUT, exist_ok=True)
built = {}
for name, (fn, pal) in SPRITES.items():
    rows = fn()
    assert len(rows) == H and all(len(r) == W for r in rows), name
    for r in rows:
        for c in r:
            assert c == '.' or pal[c] != KEY
    n = write_bmp(f'{OUT}/{name}.bmp', rows, pal)
    assert n <= 8, (name, n)
    built[name] = (rows, pal)
    print(f'{name}.bmp: 32x32, {n} palette entries incl. transparent key')

# preview: rows = main (5), normal zombie (3), rage zombie (3); 8x scale
BG = (150, 158, 168)
S, GAP = 8, 6
layout = [['main_pistol', 'main_rifle', 'main_shotgun', 'main_smg', 'main_dead'],
          ['zombie_idle', 'zombie_chase', 'zombie_dead'],
          ['rage_idle', 'rage_chase', 'rage_dead']]
cols = max(len(r) for r in layout)
cw = cols * W + (cols + 1) * GAP
ch = len(layout) * H + (len(layout) + 1) * GAP
canvas = [[BG] * cw for _ in range(ch)]
for ri, names in enumerate(layout):
    for ci, nm in enumerate(names):
        rows, pal = built[nm]
        ox, oy = GAP + ci * (W + GAP), GAP + ri * (H + GAP)
        for y in range(H):
            for x in range(W):
                c = rows[y][x]
                if c != '.':
                    canvas[oy + y][ox + x] = pal[c]
big = []
for row in canvas:
    wide = [p for p in row for _ in range(S)]
    big.extend([wide] * S)
write_png(f'{OUT}/preview.png', big)
print('ok')
