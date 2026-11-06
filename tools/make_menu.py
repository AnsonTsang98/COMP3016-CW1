import math, os, random, struct, zlib

KEY = (255, 255, 255)
OUT = '/home/claude/menu'
os.makedirs(OUT, exist_ok=True)

# ---------------------------------------------------------------- 5x7 pixel font
FONT = {
    'A': ".###./#...#/#...#/#####/#...#/#...#/#...#",
    'C': ".####/#..../#..../#..../#..../#..../.####",
    'E': "#####/#..../#..../####./#..../#..../#####",
    'G': ".####/#..../#..../#..##/#...#/#...#/.###.",
    'H': "#...#/#...#/#...#/#####/#...#/#...#/#...#",
    'I': "#####/..#../..#../..#../..#../..#../#####",
    'N': "#...#/##..#/##..#/#.#.#/#..##/#..##/#...#",
    'P': "####./#...#/#...#/####./#..../#..../#....",
    'Q': ".###./#...#/#...#/#...#/#.#.#/#..#./.##.#",
    'R': "####./#...#/#...#/####./#.#../#..#./#...#",
    'S': ".####/#..../#..../.###./....#/....#/####.",
    'T': "#####/..#../..#../..#../..#../..#../..#..",
    'U': "#...#/#...#/#...#/#...#/#...#/#...#/.###.",
    'Y': "#...#/#...#/.#.#./..#../..#../..#../..#..",
    ' ': "...../...../...../...../...../...../.....",
    '>': "#..../.#.../..#../...#./..#../.#.../#....",
    '<': "....#/...#./..#../.#.../..#../...#./....#",
    'B': "####./#...#/#...#/####./#...#/#...#/####.",
    'D': "####./#...#/#...#/#...#/#...#/#...#/####.",
    'F': "#####/#..../#..../####./#..../#..../#....",
    'J': "..###/...#./...#./...#./...#./#..#./.##..",
    'K': "#...#/#..#./#.#../##.../#.#../#..#./#...#",
    'L': "#..../#..../#..../#..../#..../#..../#####",
    'M': "#...#/##.##/#.#.#/#.#.#/#...#/#...#/#...#",
    'O': ".###./#...#/#...#/#...#/#...#/#...#/.###.",
    'V': "#...#/#...#/#...#/#...#/#...#/.#.#./..#..",
    'W': "#...#/#...#/#...#/#.#.#/#.#.#/##.##/#...#",
    'X': "#...#/#...#/.#.#./..#../.#.#./#...#/#...#",
    'Z': "#####/....#/...#./..#../.#.../#..../#####",
    '0': ".###./#...#/#..##/#.#.#/##..#/#...#/.###.",
    '1': "..#../.##../..#../..#../..#../..#../.###.",
    '2': ".###./#...#/....#/...#./..#../.#.../#####",
    '3': "####./....#/....#/.###./....#/....#/####.",
    '4': "...#./..##./.#.#./#..#./#####/...#./...#.",
    '5': "#####/#..../####./....#/....#/#...#/.###.",
    '6': ".###./#..../#..../####./#...#/#...#/.###.",
    '7': "#####/....#/...#./..#../.#.../.#.../.#...",
    '8': ".###./#...#/#...#/.###./#...#/#...#/.###.",
    '9': ".###./#...#/#...#/.####/....#/....#/.###.",
    '+': "...../..#../..#../#####/..#../..#../.....",
    '-': "...../...../...../#####/...../...../.....",
    '%': "##..#/##..#/...#./..#../.#.../#..##/#..##",
    '/': "....#/....#/...#./..#../.#.../#..../#....",
    ':': "...../..#../..#../...../..#../..#../.....",
    '.': "...../...../...../...../...../.##../.##..",
    ',': "...../...../...../...../.##../..#../.#...",
    '(': "..#../.#.../#..../#..../#..../.#.../..#..",
    ')': "..#../...#./....#/....#/....#/...#./..#..",
}


def glyph(ch):
    return [row for row in FONT[ch].split('/')]


def text_mask(text, scale, gap, ox=0, oy=0):
    m = set()
    for i, ch in enumerate(text):
        for gy, row in enumerate(glyph(ch)):
            for gx, c in enumerate(row):
                if c == '#':
                    for dy in range(scale):
                        for dx in range(scale):
                            m.add((ox + i * (5 * scale + gap) + gx * scale + dx, oy + gy * scale + dy))
    return m


def text_width(text, scale, gap):
    return len(text) * (5 * scale + gap) - gap


# ---------------------------------------------------------------- generic image helpers
class Img:
    def __init__(self, w, h, fill='.'):
        self.w, self.h = w, h
        self.g = [[fill] * w for _ in range(h)]

    def p(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.g[y][x] = c

    def r(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.p(x, y, c)

    def disc(self, cx, cy, rad, c):
        for y in range(cy - rad, cy + rad + 1):
            for x in range(cx - rad, cx + rad + 1):
                if (x - cx) ** 2 + (y - cy) ** 2 <= rad * rad + rad // 2:
                    self.p(x, y, c)

    def rows(self):
        return [''.join(r) for r in self.g]


def write_bmp(path, rows, pal_map, force_key_first=True):
    h, w = len(rows), len(rows[0])
    assert w % 4 == 0
    used = sorted({c for r in rows for c in r} - {'.'})
    chars = ['.'] + used                  # white key is ALWAYS palette index 0 (your loader keys index 0)
    assert len(chars) <= 8, (path, chars)
    idx = {c: i for i, c in enumerate(chars)}
    pal = [pal_map[c] for c in chars]
    off = 14 + 40 + 4 * len(pal)
    hdr = b'BM' + struct.pack('<IHHI', off + w * h, 0, 0, off)
    info = struct.pack('<IiiHHIIiiII', 40, w, h, 1, 8, 0, w * h, 2835, 2835, len(pal), len(pal))
    pal_b = b''.join(bytes((b, g, r, 0)) for r, g, b in pal)
    body = b''.join(bytes(idx[c] for c in rows[y]) for y in range(h - 1, -1, -1))
    with open(path, 'wb') as fh:
        fh.write(hdr + info + pal_b + body)
    for r in rows:
        for c in r:
            assert c == '.' or pal_map[c] != KEY
    return len(pal)


def write_png(path, px):
    h, w = len(px), len(px[0])
    raw = b''.join(b'\x00' + bytes(v for p in row for v in p) for row in px)

    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)

    with open(path, 'wb') as fh:
        fh.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                 + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


# ================================================================== 1. title (384 x 64)
PAL_TITLE = {'.': KEY, 'k': (10, 10, 12), 'g': (108, 108, 118), 'l': (206, 206, 212),
             'r': (176, 26, 32), 'R': (104, 12, 18)}
TW, TH = 384, 64
title = Img(TW, TH)
TEXT = "ESCAPE ANYTHING"
tx0 = (TW - text_width(TEXT, 4, 4)) // 2
ty0 = 6
mask = text_mask(TEXT, 4, 4, tx0, ty0)
for (x, y) in mask:                                     # dark outline, 2px
    for dy in range(-2, 3):
        for dx in range(-2, 3):
            if (x + dx, y + dy) not in mask:
                title.p(x + dx, y + dy, 'k')
for (x, y) in mask:                                     # letter faces, shaded underside
    shade = (x, y + 2) not in mask
    title.p(x, y, 'g' if shade else 'l')

rng = random.Random(7)
pix = sorted(mask)
for _ in range(14):                                     # blood on the letters
    x, y = rng.choice(pix)
    for dy in range(2):
        for dx in range(2):
            if (x + dx, y + dy) in mask:
                title.p(x + dx, y + dy, 'r')

bottom = {}
for (x, y) in mask:
    bottom[x] = max(bottom.get(x, -1), y)
for ci in (0, 1, 3, 5, 7, 8, 10, 12, 14):               # drips hanging from the letters
    cols = [tx0 + ci * 24 + gx * 4 + 1 for gx in range(5)]
    cols = [c for c in cols if c in bottom]
    if not cols:
        continue
    x = rng.choice(cols)
    y0 = bottom[x] - 1
    length = rng.randrange(7, 24)
    for yy in range(y0, y0 + length):
        title.p(x, yy, 'R'); title.p(x + 1, yy, 'r'); title.p(x + 2, yy, 'r')
    title.disc(x + 1, y0 + length + 1, 2, 'r')
    title.p(x, y0 + length, 'R')
n = write_bmp(f'{OUT}/menu_title.bmp', title.rows(), PAL_TITLE)
print(f'menu_title.bmp: {TW}x{TH}, {n} palette entries incl. key')

# ================================================================== 2. buttons (256 x 64: 2 frames x 2 rows)
PAL_BTN = {'.': KEY, 'k': (10, 10, 12), 'd': (28, 28, 34), 'g': (88, 88, 98), 'l': (210, 210, 216),
           'r': (190, 28, 34), 'D': (62, 8, 14)}
BW, BH = 128, 32
sheet = Img(BW * 2, BH * 4)


def button(label, hover, ox, oy):
    border = 'r' if hover else 'g'
    fill = 'D' if hover else 'd'
    sheet.r(ox + 1, oy + 1, ox + BW - 2, oy + BH - 2, 'k')
    sheet.r(ox + 2, oy + 2, ox + BW - 3, oy + BH - 3, border)
    sheet.r(ox + 4, oy + 4, ox + BW - 5, oy + BH - 5, fill)
    for (cx, cy) in ((1, 1), (BW - 2, 1), (1, BH - 2), (BW - 2, BH - 2)):      # cut corners -> transparent
        sheet.p(ox + cx, oy + cy, '.')
    tw = text_width(label, 2, 2)
    tx, ty = ox + (BW - tw) // 2, oy + (BH - 14) // 2
    for (x, y) in text_mask(label, 2, 2, tx + 1, ty + 1):
        sheet.p(x, y, 'k')
    for (x, y) in text_mask(label, 2, 2, tx, ty):
        sheet.p(x, y, 'l')
    if hover:
        for (x, y) in text_mask('>', 2, 0, ox + 12, ty):
            sheet.p(x, y, 'r')
        for (x, y) in text_mask('<', 2, 0, ox + BW - 12 - 10, ty):
            sheet.p(x, y, 'r')


for row, label in enumerate(('START', 'UPGRADE', 'QUIT', 'BACK')):
    for frame in (0, 1):
        button(label, frame == 1, frame * BW, row * BH)
n = write_bmp(f'{OUT}/menu_buttons.bmp', sheet.rows(), PAL_BTN)
print(f'menu_buttons.bmp: {BW * 2}x{BH * 4}, {n} palette entries incl. key')

# ================================================================== 3. background (640 x 320)
PAL_BG = {'.': KEY, 'k': (10, 10, 12), 'a': (24, 24, 30), 'b': (35, 35, 43), 'c': (48, 48, 57),
          'R': (84, 10, 16), 'r': (146, 20, 26)}
W, H = 640, 320
bg = Img(W, H)
cx, cy = 320, 160
LEVEL = ['k', 'a', 'b', 'c']
rng = random.Random(11)
for ty in range(H // 32):
    for tx in range(W // 32):
        x, y = tx * 32 + 16, ty * 32 + 16
        d = math.hypot((x - cx) / 300.0, (y - cy) / 150.0)
        lvl = 3 if d < 0.45 else 2 if d < 0.75 else 1 if d < 1.0 else 0
        if rng.random() < 0.22:
            lvl = max(0, lvl - 1)
        bg.r(tx * 32, ty * 32, tx * 32 + 31, ty * 32 + 31, LEVEL[lvl])
        bg.r(tx * 32, ty * 32, tx * 32 + 31, ty * 32, 'k')           # grout lines
        bg.r(tx * 32, ty * 32, tx * 32, ty * 32 + 31, 'k')
for _ in range(7):                                                    # cracks
    x, y = rng.randrange(40, W - 40), rng.randrange(30, H - 30)
    for i in range(rng.randrange(10, 26)):
        bg.p(x, y, 'k')
        x += rng.choice((1, 1, 0, -1)); y += rng.choice((1, 0, 1))


def splat(x, y, rad):
    bg.disc(x, y, rad, 'R')
    bg.disc(x, y, max(1, rad - 3), 'r')
    for _ in range(rng.randrange(5, 9)):
        a = rng.random() * math.tau
        dd = rad * rng.uniform(1.4, 2.6)
        bg.disc(int(x + math.cos(a) * dd), int(y + math.sin(a) * dd), rng.choice((1, 1, 2)), 'R')


for (x, y, r) in ((60, 252, 15), (112, 292, 8), (28, 150, 9), (588, 258, 16), (540, 300, 9),
                  (616, 138, 8), (248, 302, 7), (404, 300, 6), (24, 40, 7), (610, 36, 9)):
    splat(x, y, r)
for (x0, y0, x1, y1) in ((150, 268, 232, 262), (420, 276, 516, 282)):  # drag marks
    steps = max(abs(x1 - x0), 1)
    for i in range(steps):
        x = x0 + i
        y = round(y0 + (y1 - y0) * i / steps)
        bg.r(x, y - 1, x, y + 1, 'R')
n = write_bmp(f'{OUT}/menu_bg.bmp', bg.rows(), PAL_BG)
print(f'menu_bg.bmp: {W}x{H}, {n} palette entries incl. key (key unused, but kept at index 0)')

# ================================================================== preview: what the menu looks like in game
import make_chars as mc
S = 3
layers = []
canvas = [[PAL_BG[c] if c != '.' else (0, 0, 0) for c in row] for row in bg.rows()]


def blit(img_rows, pal, ox, oy, scale=1, rot180=False):
    rows = [r[::-1] for r in img_rows[::-1]] if rot180 else img_rows
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c != '.':
                for dy in range(scale):
                    for dx in range(scale):
                        yy, xx = oy + y * scale + dy, ox + x * scale + dx
                        if 0 <= yy < H and 0 <= xx < W:
                            canvas[yy][xx] = pal[c]


blit(mc.main_rifle(), mc.PAL_MAIN, 36, 200, 3)
blit(mc.zombie_chase(), mc.PAL_ZOMBIE, 440, 206, 3, True)
blit(mc.rage_chase(), mc.PAL_RAGE, 520, 190, 3, True)
blit(title.rows(), PAL_TITLE, (W - TW) // 2, 12)
btn = sheet.rows()
for i in range(3):
    f = 1 if i == 0 else 0
    cell = [row[f * BW:f * BW + BW] for row in btn[i * BH:(i + 1) * BH]]
    blit(cell, PAL_BTN, (W - BW) // 2, 120 + i * 44)
big = []
for row in canvas:
    wide = [p for p in row for _ in range(2)]
    big.extend([wide, wide])
write_png(f'{OUT}/menu_preview.png', big)
print('ok')
