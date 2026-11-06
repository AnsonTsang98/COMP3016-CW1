import io, contextlib
with contextlib.redirect_stdout(io.StringIO()):
    import make_menu
from make_menu import Img, write_bmp, write_png, KEY
PAL = {'.': KEY, 'k': (10, 10, 12), 'd': (34, 34, 42), 'g': (100, 100, 112), 'l': (200, 200, 208),
       'G': (70, 225, 100), 'H': (20, 90, 35)}
sheet = Img(256, 32)
for f in range(8):
    im = Img(32, 32)
    im.r(1, 1, 30, 30, 'k')                      # outline
    im.r(2, 2, 29, 29, 'g')                      # metal plate
    im.r(2, 2, 29, 2, 'l'); im.r(2, 2, 2, 29, 'l')   # lit top / left edge
    im.r(5, 5, 26, 26, 'k')                      # stair well
    for y in (9, 14, 19, 24):                    # steps going down
        im.r(6, y, 25, y, 'd')
    phase = f // 2
    for i in range(3):                           # chevrons pointing down, light travels down
        y0 = 6 + i * 7
        col = 'G' if i == phase or (phase == 3 and i == 2) else 'H'
        for j in range(5):
            for x in (11 + j, 12 + j, 20 - j, 21 - j):
                im.p(x, y0 + j, col)
    lamp = 'G' if (f % 4) < 2 else 'H'           # corner lamps blink
    for (x, y) in ((3, 3), (28, 3), (3, 28), (28, 28)):
        im.p(x, y, lamp)
    for y in range(32):
        for x in range(32):
            sheet.g[y][x + f * 32] = im.g[y][x]
n = write_bmp('/home/claude/map/exit_anim.bmp', sheet.rows(), PAL)
print(n)
Z = 3
rows = sheet.rows()
px = [[PAL[c] if c != '.' else (30, 30, 37) for c in r] for r in rows]
big = []
for r in px:
    w = [p for p in r for _ in range(Z)]
    big += [w] * Z
write_png('/home/claude/map/exit_preview.png', big)
