import io, contextlib
with contextlib.redirect_stdout(io.StringIO()):
    import make_upgrade as mu
from make_menu import text_mask, write_png, PAL_BTN, PAL_TITLE, PAL_BG, bg, OUT, W, H, sheet, BW, BH
canvas = [[PAL_BG[c] if c != '.' else (0, 0, 0) for c in row] for row in bg.rows()]
for y in range(H):                                   # dark overlay, same as the game draws (alpha 150)
    for x in range(W):
        canvas[y][x] = tuple(int(v * 0.41) for v in canvas[y][x])

def blit(rows, pal, ox, oy):
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c != '.' and 0 <= oy + y < H and 0 <= ox + x < W:
                canvas[oy + y][ox + x] = pal[c]

def dtext(s, x, y, col):
    for (px, py) in text_mask(s.upper(), 1, 3, x, y):
        if 0 <= py < H and 0 <= px < W: canvas[py][px] = col

blit(mu.t.rows(), PAL_TITLE, (W - 256) // 2, 6)
score = 47
st = "SCORE: %d" % score
dtext(st, (W - len(st) * 8) // 2, 52, (210, 210, 216))
levels, maxl, base = [2, 1, 0, 3], [5, 5, 3, 5], [10, 10, 15, 20]
info = ["%d > %d HP" % (100 + 20 * 2, 100 + 20 * 3), "x%.1f > x%.1f" % (1.1, 1.2),
        "RADIUS %d > %d" % (4, 5), "DMG x%.2f > x%.2f" % (1.15, 1.2)]
# fix: show the *next* level effect
info = ["%d > %d" % (100 + 20 * 2, 100 + 20 * 3), "x%.1f > x%.1f" % (1.0 + .1 * 1, 1.0 + .1 * 2),
        "RADIUS %d > %d" % (4, 5), "DAMAGE +%d%% > +%d%%" % (30, 40)]
rows = mu.rows.rows()
icons = mu.icons.rows()
plus = mu.plus.rows()
pips = mu.pips.rows()
for i in range(4):
    sel = 1 if i == 0 else 0
    ox, oy = 80, 64 + i * 44
    blit(rows[(sel * 4 + i) * 40:(sel * 4 + i + 1) * 40], PAL_BTN, ox, oy)
    blit([r[i * 32:i * 32 + 32] for r in icons], mu.PAL_ICON, ox + 4, oy + 4)
    dtext(info[i], ox + 44, oy + 25, (150, 150, 160))
    for k in range(maxl[i]):
        f = 1 if k < levels[i] else 0
        blit([r[f * 8:f * 8 + 8] for r in pips], PAL_BTN, ox + 300 + k * 10, oy + 16)
    cost = base[i] * (levels[i] + 1)
    if levels[i] >= maxl[i]:
        fr = 2; label = "MAX"
    else:
        fr = 0 if score >= cost else 1
        label = "COST %d" % cost
    bx = ox + 480 - 36
    blit([r[fr * 32:fr * 32 + 32] for r in plus], PAL_BTN, bx, oy + 8)
    if fr != 2:
        dtext(label, bx - 4 - len(label) * 8, oy + 16, (210, 210, 216) if fr == 0 else (120, 120, 130))
btn = sheet.rows()
blit([row[0:BW] for row in btn[3 * BH:4 * BH]], PAL_BTN, (W - BW) // 2, 266)
hint = "W/S or MOUSE: select   ENTER or CLICK: buy   ESC: back"
dtext(hint, (W - len(hint) * 8) // 2, H - 14, (120, 120, 130))
big = []
for row in canvas:
    wide = [p for p in row for _ in range(2)]
    big.extend([wide, wide])
write_png(f'{OUT}/upgrade_preview.png', big)
print('ok')
