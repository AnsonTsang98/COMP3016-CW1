import io, contextlib, random, math
from collections import deque
with contextlib.redirect_stdout(io.StringIO()):
    import make_menu as mm
from make_menu import Img, write_bmp, write_png, KEY
OUT = '/home/claude/map'
ROWS, COLS = 30, 50

# ================================================================== tiles (32x32, item spec)
PAL_G = {'.': KEY, 'k': (10, 10, 12), 'a': (30, 30, 37), 'b': (40, 40, 49), 'c': (52, 52, 62), 'd': (24, 24, 30)}
rng = random.Random(21)
g = Img(32, 32)
g.r(0, 0, 31, 31, 'b')
for _ in range(60):                                    # speckle
    g.p(rng.randrange(32), rng.randrange(32), rng.choice('aac'))
for (x, y, w, h) in ((4, 5, 7, 2), (18, 20, 8, 2), (21, 6, 4, 1), (6, 24, 5, 1)):   # worn patches
    for yy in range(h):
        for xx in range(w):
            if rng.random() < 0.75:
                g.p(x + xx, y + yy, 'c')
x, y = 12, 12                                           # one small crack
for i in range(9):
    g.p(x, y, 'k'); x += rng.choice((1, 1, 0)); y += rng.choice((1, 0, 1))
g.r(0, 0, 31, 0, 'k'); g.r(0, 0, 0, 31, 'k')            # grout (top / left)
g.r(1, 1, 30, 1, 'd'); g.r(1, 1, 1, 30, 'd')
n = write_bmp(f'{OUT}/ground3.bmp', g.rows(), PAL_G)
print('ground3', n)

PAL_W = {'.': KEY, 'k': (10, 10, 12), 'd': (34, 34, 42), 'a': (66, 66, 77), 'b': (80, 80, 93), 'c': (122, 122, 136)}
w = Img(32, 32)
w.r(0, 0, 31, 31, 'a')
for row in range(4):                                    # 4 courses of bricks, 8px high, staggered
    y0 = row * 8
    off = 0 if row % 2 == 0 else 8
    for bx in range(-16, 40, 16):
        x0 = bx + off
        shade = 'b' if (row * 3 + bx // 16) % 3 == 0 else 'a'
        for yy in range(y0 + 1, y0 + 7):
            for xx in range(x0 + 1, x0 + 15):
                if 0 <= xx < 32:
                    w.p(xx, yy, shade)
        for xx in range(x0 + 1, x0 + 15):               # brick top highlight, bottom shade
            if 0 <= xx < 32:
                w.p(xx, y0 + 1, 'c'); w.p(xx, y0 + 6, 'd')
    w.r(0, y0, 31, y0, 'k')                             # mortar lines
    for bx in range(-16, 40, 16):
        xm = bx + off
        if 0 <= xm < 32:
            w.r(xm, y0, xm, y0 + 7, 'k')
w.r(0, 31, 31, 31, 'k')
w.r(0, 0, 0, 31, 'k')
n = write_bmp(f'{OUT}/wall2.bmp', w.rows(), PAL_W)
print('wall2', n)

# ================================================================== map generator
def build(seed):
    r = random.Random(seed)
    m = [[0] * COLS for _ in range(ROWS)]
    for c in range(COLS): m[0][c] = m[ROWS - 1][c] = 5
    for y in range(ROWS): m[y][0] = m[y][COLS - 1] = 5
    leaves, walls = [], []                              # walls: (orientation, line, a, b)

    def split(r0, c0, r1, c1):
        h, wd = r1 - r0 + 1, c1 - c0 + 1
        can_h, can_v = h >= 2 * 6 + 1, wd >= 2 * 8 + 1
        if not (can_h or can_v) or (h * wd < 90 and r.random() < 0.6):
            leaves.append((r0, c0, r1, c1)); return
        horiz = can_h if not can_v else (not can_h and False) or (h / wd > 0.5 if can_h else False) or not can_h and False
        if can_h and can_v: horiz = r.random() < (h / (h + wd) + 0.2)
        elif can_h: horiz = True
        else: horiz = False
        if horiz:
            ln = r.randint(r0 + 5, r1 - 5)
            for c in range(c0, c1 + 1): m[ln][c] = 5
            walls.append(('h', ln, c0, c1))
            split(r0, c0, ln - 1, c1); split(ln + 1, c0, r1, c1)
        else:
            ln = r.randint(c0 + 7, c1 - 7)
            for y in range(r0, r1 + 1): m[y][ln] = 5
            walls.append(('v', ln, r0, r1))
            split(r0, c0, r1, ln - 1); split(r0, ln + 1, r1, c1)
    split(1, 1, ROWS - 2, COLS - 2)

    opening = set()
    base = [row[:] for row in m]
    for (o, ln, a, b) in walls:                         # carve a 2-wide opening with free cells on both sides
        cands = []
        for p in range(a + 1, b - 1):
            cells = [(ln, p), (ln, p + 1)]
            if o == 'h':
                side = [(ln - 1, p), (ln - 1, p + 1), (ln + 1, p), (ln + 1, p + 1)]
            else:
                cells = [(p, ln), (p + 1, ln)]
                side = [(p, ln - 1), (p + 1, ln - 1), (p, ln + 1), (p + 1, ln + 1)]
            if all(base[y][x] == 5 for y, x in cells) and all(base[y][x] == 0 for y, x in side):
                cands.append(cells)
        assert cands, ('no opening', o, ln)
        cells = r.choice(cands)
        for y, x in cells:
            m[y][x] = 0; opening.add((y, x))
    near_open = {(y + dy, x + dx) for (y, x) in opening for dy in (-2, -1, 0, 1, 2) for dx in (-2, -1, 0, 1, 2)}

    # pillars (2x2 cover blocks) in big rooms
    for (r0, c0, r1, c1) in leaves:
        if (r1 - r0 + 1) >= 8 and (c1 - c0 + 1) >= 12:
            for _ in range(r.randint(1, 3)):
                y, x = r.randint(r0 + 2, r1 - 3), r.randint(c0 + 2, c1 - 3)
                cells = [(y, x), (y + 1, x), (y, x + 1), (y + 1, x + 1)]
                if all(m[a][b] == 0 and (a, b) not in near_open for a, b in cells):
                    for a, b in cells: m[a][b] = 5
    return m, leaves, near_open

def reach(m, start):
    seen = {start}; q = deque([start])
    while q:
        y, x = q.popleft()
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            ny, nx = y + dy, x + dx
            if m[ny][nx] != 5 and (ny, nx) not in seen:
                seen.add((ny, nx)); q.append((ny, nx))
    return seen

for seed in range(1, 200):
    m, leaves, near_open = build(seed)
    floor = [(y, x) for y in range(ROWS) for x in range(COLS) if m[y][x] == 0]
    if len(reach(m, (1, 1)) & set(floor)) == len(floor) and 12 <= len(leaves) <= 22:
        break
else:
    raise SystemExit('no seed')
print('seed', seed, 'rooms', len(leaves))

r = random.Random(seed * 13 + 5)
start_room = [l for l in leaves if l[0] == 1 and l[1] == 1][0]
m[2][2] = 4
pr = (2, 2)
used = {pr}
def free_in(room, margin=1):
    r0, c0, r1, c1 = room
    for _ in range(500):
        y, x = r.randint(r0 + margin, r1 - margin), r.randint(c0 + margin, c1 - margin)
        if m[y][x] == 0 and (y, x) not in used and (y, x) not in near_open:
            return (y, x)
# exit door: far corner of the room that holds (ROWS-2, COLS-2)
assert m[ROWS - 2][COLS - 2] == 0
m[ROWS - 2][COLS - 2] = 7; used.add((ROWS - 2, COLS - 2))        # 7 = exit
# items: 4 near the start, then 1-2 per other room
items = 0
for dy, dx in ((0, 3), (1, 5), (3, 3), (4, 5)):
    y, x = start_room[0] + 1 + dy, start_room[1] + 1 + dx
    if start_room[0] <= y <= start_room[2] and start_room[1] <= x <= start_room[3] and m[y][x] == 0 and (y, x) not in near_open:
        m[y][x] = 6; used.add((y, x)); items += 1
enemies = 0
for room in leaves:
    if room == start_room:
        continue
    for _ in range(r.randint(1, 2)):
        p = free_in(room)
        if p: m[p[0]][p[1]] = 6; used.add(p); items += 1
    for _ in range(r.randint(1, 3)):
        p = free_in(room)
        if p and math.hypot(p[0] - pr[0], p[1] - pr[1]) >= 14:
            far = math.hypot(p[0] - pr[0], p[1] - pr[1])
            rage = far >= 24 and r.random() < 0.45            # 2 = rage, only far from the start
            m[p[0]][p[1]] = 2 if rage else 3; used.add(p); enemies += 1
print('items', items, 'enemies', enemies)

# validation
flat = [m[y][x] for y in range(ROWS) for x in range(COLS)]
assert flat.count(4) == 1 and flat.count(7) == 1 and flat.count(2) >= 3
rc = reach(m, pr)
for y in range(ROWS):
    for x in range(COLS):
        if m[y][x] in (2, 3, 4, 6, 7): assert (y, x) in rc, (y, x, m[y][x])
for y in range(ROWS):
    for x in range(COLS):
        if y in (0, ROWS - 1) or x in (0, COLS - 1): assert m[y][x] == 5
bgm = [[0 if m[y][x] == 5 else 1 for x in range(COLS)] for y in range(ROWS)]

with open(f'{OUT}/map_30x50.txt', 'w') as f:
    f.write('    short map[MAP_ROWS][MAP_COLS] = {\n')
    for row in m: f.write('        ' + ', '.join(str(v) for v in row) + ',\n')
    f.write('    };\n    //background map\n    short backgroundMap[MAP_ROWS][MAP_COLS] = {\n')
    for row in bgm: f.write('        ' + ', '.join(str(v) for v in row) + ',\n')
    f.write('    };\n')
with open(f'{OUT}/map_30x50_ascii.txt', 'w') as f:
    sym = {0: '.', 2: 'R', 3: 'Z', 4: 'P', 5: '#', 6: 'i', 7: 'E'}
    for row in m: f.write(''.join(sym[v] for v in row) + '\n')

# preview (1 px per tile pixel -> 1600x960, halved to 800x480 for viewing)
S = 16
canvas = []
gr, wr = g.rows(), w.rows()
for ty in range(ROWS):
    for yy in range(32):
        line = []
        for tx in range(COLS):
            if m[ty][tx] == 5: line.extend(PAL_W[c] for c in wr[yy])
            else: line.extend(PAL_G[c] for c in gr[yy])
        canvas.append(line)
def mark(ty, tx, col):
    for yy in range(8, 24):
        for xx in range(8, 24):
            if (xx - 15.5) ** 2 + (yy - 15.5) ** 2 <= 56:
                canvas[ty * 32 + yy][tx * 32 + xx] = col
for ty in range(ROWS):
    for tx in range(COLS):
        v = m[ty][tx]
        if v == 4: mark(ty, tx, (240, 240, 240))
        elif v == 3: mark(ty, tx, (60, 150, 60))
        elif v == 2: mark(ty, tx, (210, 50, 50))
        elif v == 6: mark(ty, tx, (70, 130, 230))
        elif v == 7:
            for yy in range(4, 28):
                for xx in range(4, 28): canvas[ty * 32 + yy][tx * 32 + xx] = (60, 220, 90)
small = [[canvas[y][x] for x in range(0, COLS * 32, 2)] for y in range(0, ROWS * 32, 2)]
write_png(f'{OUT}/map_preview.png', small)
print('ok')
