import io, contextlib, random
with contextlib.redirect_stdout(io.StringIO()):
    import make_menu as mm
from make_menu import Img, write_bmp, write_png, text_mask, text_width, PAL_BTN, PAL_TITLE, PAL_BG, KEY, OUT

# ---------------------------------------------------------------- outline helper
def outline(img, c='k'):
    g = img.g
    add = []
    for y in range(img.h):
        for x in range(img.w):
            if g[y][x] == '.':
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < img.w and 0 <= yy < img.h and g[yy][xx] not in ('.', c):
                        add.append((x, y)); break
    for x, y in add:
        g[y][x] = c

def stamp(img, pattern, ox, oy, sc, col):
    for y, row in enumerate(pattern):
        for x, ch in enumerate(row):
            if ch == '#':
                img.r(ox + x * sc, oy + y * sc, ox + x * sc + sc - 1, oy + y * sc + sc - 1, col)

# ================================================================== 1. icons (128 x 32, 4 frames of 32x32)
PAL_ICON = {'.': KEY, 'k': (10, 10, 12), 'g': (110, 110, 120), 'l': (210, 210, 216),
            'r': (190, 28, 34), 'R': (110, 14, 20), 'G': (70, 160, 70), 'd': (40, 40, 48)}
icons = Img(128, 32)

# 0: heart (max HP)
HEART = ["..###...###..", ".#####.#####.", "#############", "#############", ".###########.",
         "..#########..", "...#######...", "....#####....", ".....###.....", "......#......"]
h = Img(32, 32)
stamp(h, HEART, 3, 6, 2, 'r')
for y in range(h.h):                                   # shaded lower-right half
    for x in range(h.w):
        if h.g[y][x] == 'r' and x + y > 40:
            h.g[y][x] = 'R'
h.r(7, 10, 10, 13, 'l')
outline(h)
# 1: speed chevrons
s = Img(32, 32)
for i, col in enumerate(('g', 'l', 'l')):
    x0 = 6 + i * 7
    for j in range(8):
        s.r(x0 + j, 6 + j, x0 + j + 2, 6 + j, col)
        s.r(x0 + j, 22 - j, x0 + j + 2, 22 - j, col)
for yy, (a, b) in ((10, (1, 8)), (15, (0, 10)), (20, (2, 9))):
    s.r(a, yy, b, yy + 1, 'g')
outline(s)
# 2: eye
e = Img(32, 32)
for y in range(32):
    for x in range(32):
        if ((x - 15.5) / 14.5) ** 2 + ((y - 15.5) / 8.5) ** 2 <= 1.0:
            e.p(x, y, 'l')
e.disc(16, 16, 6, 'G')
e.disc(16, 16, 3, 'k')
e.r(12, 12, 13, 13, 'l')
outline(e)
# 3: pistol
p = Img(32, 32)
p.r(4, 10, 27, 15, 'l')
p.r(4, 14, 27, 15, 'g')
p.r(12, 16, 19, 18, 'g')
for y in range(16, 27):
    off = (y - 16) // 3
    p.r(7 + off, y, 12 + off, y, 'g')
p.r(13, 19, 13, 21, 'g'); p.r(13, 21, 18, 21, 'g'); p.r(18, 18, 18, 21, 'g')
p.p(5, 9, 'l'); p.p(26, 9, 'l')
outline(p)
p.r(28, 11, 30, 14, 'r'); p.r(29, 12, 30, 13, 'l')
for k, im in enumerate((h, s, e, p)):
    for y in range(32):
        for x in range(32):
            icons.g[y][x + k * 32] = im.g[y][x]
n = write_bmp(f'{OUT}/upgrade_icons.bmp', icons.rows(), PAL_ICON)
print('icons', n)

# ================================================================== 2. rows (480 x 320): 4 rows x (normal, selected)
RW, RH = 480, 40
LABELS = ('MAX HP', 'SPEED', 'VISION', 'FIREARMS')
rows = Img(RW, RH * 8)
for sel in (0, 1):
    for r, label in enumerate(LABELS):
        ox, oy = 0, (sel * 4 + r) * RH
        border, fill = ('r', 'D') if sel else ('g', 'd')
        rows.r(ox + 1, oy + 1, ox + RW - 2, oy + RH - 2, 'k')
        rows.r(ox + 2, oy + 2, ox + RW - 3, oy + RH - 3, border)
        rows.r(ox + 4, oy + 4, ox + RW - 5, oy + RH - 5, fill)
        for cx, cy in ((1, 1), (RW - 2, 1), (1, RH - 2), (RW - 2, RH - 2)):
            rows.p(ox + cx, oy + cy, '.')
        rows.r(ox + 3, oy + 3, ox + 40, oy + 36, 'k')          # icon well (icon is drawn on top at +4,+4)
        for (x, y) in text_mask(label, 2, 2, ox + 45, oy + 7):
            rows.p(x, y, 'k')
        for (x, y) in text_mask(label, 2, 2, ox + 44, oy + 6):
            rows.p(x, y, 'l')
n = write_bmp(f'{OUT}/upgrade_rows.bmp', rows.rows(), PAL_BTN)
print('rows', n)

# ================================================================== 3. pips (16 x 8) and plus/max button (96 x 24)
pips = Img(16, 8)
pips.r(0, 0, 7, 7, 'g'); pips.r(1, 1, 6, 6, 'd')
pips.r(8, 0, 15, 7, 'D'); pips.r(9, 1, 14, 6, 'r'); pips.r(10, 2, 11, 3, 'l')
n = write_bmp(f'{OUT}/upgrade_pips.bmp', pips.rows(), PAL_BTN)
print('pips', n)

plus = Img(96, 24)
def pbtn(f, border, fill, label, scale, col):
    ox = f * 32
    plus.r(ox, 0, ox + 31, 23, 'k')
    plus.r(ox + 1, 1, ox + 30, 22, border)
    plus.r(ox + 3, 3, ox + 28, 20, fill)
    tw = text_width(label, scale, scale)
    tx = ox + (32 - tw) // 2
    ty = (24 - 7 * scale) // 2
    for (x, y) in text_mask(label, scale, scale, tx, ty):
        plus.p(x, y, col)
    for c in ((0, 0), (31, 0), (0, 23), (31, 23)):
        plus.p(ox + c[0], c[1], '.')
pbtn(0, 'r', 'D', '+', 2, 'l')       # can afford
pbtn(1, 'g', 'd', '+', 2, 'g')       # cannot afford
pbtn(2, 'g', 'd', 'MAX', 1, 'l')     # maxed
n = write_bmp(f'{OUT}/upgrade_plus.bmp', plus.rows(), PAL_BTN)
print('plus', n)

# ================================================================== 4. title (256 x 48)
TW2, TH2 = 256, 48
t = Img(TW2, TH2)
text = 'UPGRADES'
tw = text_width(text, 4, 4)
tx, ty = (TW2 - tw) // 2, 4
mask = text_mask(text, 4, 4, tx, ty)
for (x, y) in mask:
    t.p(x, y, 'g' if (x, y + 2) not in mask else 'l')
outline(t)
rng = random.Random(5)
cols = sorted({x for (x, y) in mask})
for x0 in (tx + 0 * 24 + 8, tx + 2 * 24 + 8, tx + 3 * 24 + 2, tx + 5 * 24 + 2, tx + 7 * 24 + 2):
    ln = rng.randrange(6, 13)
    t.r(x0, ty + 27, x0, ty + 28 + ln, 'r')
    t.r(x0 - 1, ty + 28 + ln, x0 + 1, ty + 30 + ln, 'r')
    t.p(x0, ty + 31 + ln, 'R')
n = write_bmp(f'{OUT}/upgrade_title.bmp', t.rows(), PAL_TITLE)
print('title', n)
open('/home/claude/menu/.ok', 'w').write('ok')
