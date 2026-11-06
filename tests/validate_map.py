"""Checks the level arrays in CW1.cpp (python3 validate_map.py [path to CW1.cpp]).

Map codes: 1 ground, 2 rage zombie, 3 normal zombie, 4 player, 5 wall, 6 item, 7 exit, 0 empty.
Checks: size 30 x 50, solid border, exactly one player and one exit, and that the player can walk
to the exit, to every item and to every zombie (4-neighbour flood fill, walls block).
"""
import re, sys
from collections import deque

ROWS, COLS = 30, 50
path = sys.argv[1] if len(sys.argv) > 1 else '../CW1.cpp'
src = open(path, encoding='utf-8', errors='replace').read()

def grid(name):
    m = re.search(r'short ' + name + r'\[MAP_ROWS\]\[MAP_COLS\]\s*=\s*\{(.*?)\};', src, re.S)
    assert m, f'{name} not found in {path}'
    nums = [int(v) for v in re.findall(r'-?\d+', re.sub(r'//.*', '', m.group(1)))]
    assert len(nums) == ROWS * COLS, f'{name}: {len(nums)} values, expected {ROWS * COLS}'
    return [nums[r * COLS:(r + 1) * COLS] for r in range(ROWS)]

m = grid('map')
bg = grid('backgroundMap')
flat = [v for row in m for v in row]
assert flat.count(4) == 1, 'need exactly one player (4)'
assert flat.count(7) == 1, 'need exactly one exit (7)'
assert set(flat) <= {0, 2, 3, 4, 5, 6, 7}, 'unknown tile code'
for y in range(ROWS):
    for x in range(COLS):
        if y in (0, ROWS - 1) or x in (0, COLS - 1):
            assert m[y][x] == 5, f'border is not a wall at {y},{x}'
        assert bg[y][x] == (0 if m[y][x] == 5 else 1), f'background mismatch at {y},{x}'   # ground under everything except walls

start = next((y, x) for y in range(ROWS) for x in range(COLS) if m[y][x] == 4)
seen = {start}
q = deque([start])
while q:
    y, x = q.popleft()
    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        ny, nx = y + dy, x + dx
        if 0 <= ny < ROWS and 0 <= nx < COLS and m[ny][nx] != 5 and (ny, nx) not in seen:
            seen.add((ny, nx)); q.append((ny, nx))
targets = [(y, x) for y in range(ROWS) for x in range(COLS) if m[y][x] in (2, 3, 6, 7)]
unreachable = [t for t in targets if t not in seen]
assert not unreachable, f'unreachable: {unreachable}'
print(f'validate_map: ok  ({flat.count(5)} walls, {flat.count(6)} items, {flat.count(2)} rage + {flat.count(3)} normal zombies, '
      f'all {len(targets)} targets reachable from the player)')
