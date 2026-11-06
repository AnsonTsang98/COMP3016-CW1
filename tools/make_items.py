import struct, zlib, os

W = H = 32
KEY = (255, 255, 255)  # transparent colour key
PAL = {
    '.': KEY,
    'k': (24, 24, 28),
    'd': (62, 66, 74),
    'm': (112, 118, 128),
    'l': (172, 178, 188),
    'r': (204, 44, 44),
    'R': (140, 26, 30),
    'c': (238, 232, 214),   # cream (NOT pure white)
    'y': (250, 214, 70),
    'W': (124, 78, 42),
    'w': (168, 114, 62),
}


class Sprite:
    def __init__(self):
        self.p = {}

    def px(self, x, y, c):
        self.p[(x, y)] = c

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.p[(x, y)] = c

    def clear(self, x0, y0, x1, y1):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.p.pop((x, y), None)

    def finish(self):
        xs = [x for x, _ in self.p]
        ys = [y for _, y in self.p]
        w = max(xs) - min(xs) + 1 + 2   # +2 for outline
        h = max(ys) - min(ys) + 1 + 2
        assert w <= W and h <= H, (w, h)
        ox = (W - w) // 2 + 1 - min(xs)
        oy = (H - h) // 2 + 1 - min(ys)
        fill = {(x + ox, y + oy): c for (x, y), c in self.p.items()}
        out = dict(fill)
        for (x, y) in fill:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                q = (x + dx, y + dy)
                if q not in fill:
                    out[q] = 'k'
        grid = [['.'] * W for _ in range(H)]
        for (x, y), c in out.items():
            assert 0 <= x < W and 0 <= y < H, (x, y)
            grid[y][x] = c
        return [''.join(r) for r in grid]


def medkit():
    s = Sprite()
    s.rect(11, 6, 20, 9, 'm')          # handle
    s.clear(13, 7, 18, 8)
    s.rect(5, 10, 26, 25, 'r')         # box
    s.rect(5, 24, 26, 25, 'R')         # shadow band
    s.rect(14, 12, 17, 23, 'c')        # cross vertical
    s.rect(10, 16, 21, 19, 'c')        # cross horizontal
    s.rect(7, 12, 8, 12, 'R')          # little latch marks
    s.rect(23, 12, 24, 12, 'R')
    return s.finish()


def flashlight():
    s = Sprite()
    s.rect(5, 13, 19, 18, 'm')         # body
    s.rect(5, 13, 19, 13, 'l')         # highlight
    s.rect(5, 18, 19, 18, 'd')         # shade
    for x in (9, 13, 17):
        s.rect(x, 14, x, 18, 'd')      # grip rings
    s.rect(3, 14, 4, 17, 'd')          # tail cap
    s.rect(10, 12, 12, 12, 'r')        # switch
    s.rect(20, 12, 21, 19, 'm')        # neck
    s.rect(22, 10, 26, 21, 'd')        # head
    s.rect(22, 10, 26, 10, 'm')
    s.rect(27, 12, 28, 19, 'y')        # lens
    s.rect(27, 12, 27, 13, 'l')
    return s.finish()


def pistol():
    s = Sprite()
    s.rect(5, 10, 26, 14, 'd')         # slide
    s.rect(6, 10, 25, 10, 'm')
    s.rect(6, 9, 7, 9, 'd')            # rear sight
    s.rect(25, 9, 26, 9, 'd')          # front sight
    for x in (8, 10, 12):
        s.rect(x, 11, x, 13, 'm')      # rear serrations
    s.rect(26, 11, 26, 13, 'm')        # muzzle
    s.rect(14, 15, 25, 16, 'm')        # frame rail
    grip = {15: (8, 14), 16: (8, 14), 17: (8, 14), 18: (7, 13),
            19: (7, 13), 20: (7, 13), 21: (6, 12)}
    for y, (a, b) in grip.items():
        s.rect(a, y, b, y, 'd')
        if y in (17, 19):
            s.px(a + 2, y, 'm')        # grip texture
            s.px(a + 4, y, 'm')
    s.rect(6, 21, 12, 21, 'm')         # magazine base
    s.rect(15, 17, 19, 17, 'd')        # trigger guard
    s.px(19, 18, 'd')
    s.rect(14, 19, 19, 19, 'd')
    return s.finish()


def assault_rifle():
    s = Sprite()
    s.rect(3, 11, 8, 16, 'd')          # stock
    s.rect(2, 12, 3, 16, 'm')          # butt pad
    s.rect(9, 10, 21, 16, 'd')         # receiver
    s.rect(10, 15, 20, 16, 'm')        # lower receiver
    s.rect(9, 9, 25, 9, 'm')           # top rail
    s.rect(12, 6, 17, 8, 'd')          # optic
    s.rect(17, 6, 17, 8, 'l')          # lens
    s.rect(22, 10, 28, 15, 'm')        # handguard
    for x in (24, 26, 28):
        s.px(x, 12, 'd')               # vents
        s.px(x, 13, 'd')
    s.rect(29, 10, 30, 13, 'd')        # muzzle device
    s.rect(30, 11, 30, 12, 'l')
    grip = {17: (10, 13), 18: (10, 13), 19: (10, 13), 20: (9, 12),
            21: (9, 12), 22: (9, 12)}
    for y, (a, b) in grip.items():
        s.rect(a, y, b, y, 'd')
    mag = {17: (16, 19), 18: (16, 19), 19: (16, 19), 20: (17, 20),
           21: (17, 20), 22: (17, 20), 23: (17, 20)}
    for y, (a, b) in mag.items():
        s.rect(a, y, b, y, 'm')
    return s.finish()


def smg():
    s = Sprite()
    s.rect(2, 11, 3, 15, 'm')          # butt pad
    s.rect(4, 12, 7, 14, 'd')          # stock
    s.rect(8, 10, 23, 16, 'd')         # body
    s.rect(8, 9, 23, 9, 'm')           # top
    s.rect(9, 8, 10, 8, 'd')           # rear sight
    s.rect(22, 8, 23, 8, 'd')          # front sight
    s.rect(13, 12, 18, 13, 'm')        # ejection port
    s.px(15, 10, 'l')                  # charging handle
    s.rect(24, 11, 28, 15, 'm')        # forend
    s.rect(29, 12, 30, 13, 'l')        # muzzle
    grip = {17: (9, 12), 18: (9, 12), 19: (8, 11), 20: (8, 11),
            21: (7, 10), 22: (7, 10)}
    for y, (a, b) in grip.items():
        s.rect(a, y, b, y, 'd')
    s.rect(16, 17, 19, 24, 'm')        # straight magazine
    s.rect(16, 22, 19, 22, 'd')
    s.rect(13, 17, 15, 17, 'd')        # trigger guard
    return s.finish()


def shotgun():
    s = Sprite()
    stock = {12: (8, 11), 13: (6, 11), 14: (4, 11), 15: (3, 11),
             16: (2, 11), 17: (2, 10), 18: (2, 9), 19: (2, 8)}
    for y, (a, b) in stock.items():
        s.rect(a, y, b, y, 'W')
        s.px(a, y, 'w')                # wood highlight edge
    s.rect(12, 11, 20, 15, 'd')        # receiver
    s.rect(12, 10, 20, 10, 'm')
    s.rect(15, 12, 18, 13, 'k')        # ejection port
    s.rect(21, 11, 30, 12, 'm')        # barrel
    s.rect(21, 11, 30, 11, 'l')
    s.rect(21, 13, 29, 14, 'd')        # magazine tube
    s.rect(29, 13, 29, 14, 'l')        # tube cap
    s.rect(20, 13, 26, 16, 'W')        # pump forend
    s.rect(20, 13, 26, 13, 'w')
    s.px(30, 10, 'l')                  # bead sight
    s.rect(12, 16, 18, 16, 'd')        # trigger guard
    s.px(12, 17, 'd')
    s.px(18, 17, 'd')
    s.rect(12, 18, 18, 18, 'd')
    return s.finish()


ITEMS = {
    "medkit": medkit(),
    "flashlight": flashlight(),
    "pistol": pistol(),
    "assault_rifle": assault_rifle(),
    "smg": smg(),
    "shotgun": shotgun(),
}


def write_bmp(path, rows):
    used = sorted({c for r in rows for c in r}, key=lambda c: (c != '.', c))
    assert used[0] == '.'
    idx = {c: i for i, c in enumerate(used)}
    pal = [PAL[c] for c in used]
    off = 14 + 40 + 4 * len(pal)
    size = off + W * H
    hdr = b'BM' + struct.pack('<IHHI', size, 0, 0, off)
    info = struct.pack('<IiiHHIIiiII', 40, W, H, 1, 8, 0, W * H, 2835, 2835,
                       len(pal), len(pal))
    pal_b = b''.join(bytes((b, g, r, 0)) for r, g, b in pal)
    body = b''.join(bytes(idx[c] for c in rows[y]) for y in range(H - 1, -1, -1))
    with open(path, 'wb') as f:
        f.write(hdr + info + pal_b + body)
    return len(pal)


def write_png(path, px):
    h, w = len(px), len(px[0])
    raw = b''.join(b'\x00' + bytes(v for p in row for v in p) for row in px)

    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)

    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n'
                + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


os.makedirs("/home/claude/items", exist_ok=True)
for name, rows in ITEMS.items():
    assert len(rows) == H and all(len(r) == W for r in rows), name
    n = write_bmp(f"/home/claude/items/{name}.bmp", rows)
    # every non-background pixel must differ from the colour key
    for r in rows:
        for c in r:
            if c != '.':
                assert PAL[c] != KEY
    print(f"{name}: {n} palette entries (incl. transparent key)")

# preview (key colour shown as light blue-grey), 5x scale, 4px gutters
BG = (200, 214, 230)
GAP = 4
n = len(ITEMS)
canvas_w = n * W + (n + 1) * GAP
canvas_h = H + 2 * GAP
canvas = [[BG] * canvas_w for _ in range(canvas_h)]
for i, rows in enumerate(ITEMS.values()):
    x0 = GAP + i * (W + GAP)
    for y in range(H):
        for x in range(W):
            c = rows[y][x]
            if c != '.':
                canvas[GAP + y][x0 + x] = PAL[c]
S = 5
big = []
for row in canvas:
    wide = [p for p in row for _ in range(S)]
    big.extend([wide] * S)
write_png("/home/claude/items/preview.png", big)
print("ok")
