import numpy as np, wave, os
from scipy.signal import butter, sosfilt

SR = 44100
OUT = '/home/claude/sounds'
rng = np.random.default_rng(7)

def t_of(d): return np.arange(int(SR * d)) / SR
def lp(x, fc, order=2): return sosfilt(butter(order, fc, 'low', fs=SR, output='sos'), x)
def hp(x, fc, order=2): return sosfilt(butter(order, fc, 'high', fs=SR, output='sos'), x)
def bp(x, lo, hi, order=2): return sosfilt(butter(order, [lo, hi], 'band', fs=SR, output='sos'), x)
def noise(n): return rng.standard_normal(n)
def env_ar(n, a, r):                                   # attack / release envelope (seconds)
    e = np.ones(n); na, nr = int(a * SR), int(r * SR)
    if na: e[:na] = np.linspace(0, 1, na) ** 1.5
    if nr: e[-nr:] *= np.linspace(1, 0, nr) ** 1.5
    return e
def norm(x, peak=0.85): return x / (np.max(np.abs(x)) + 1e-9) * peak
def fade(x, ms=4):
    n = int(SR * ms / 1000); x = x.copy(); x[:n] *= np.linspace(0, 1, n); x[-n:] *= np.linspace(1, 0, n); return x
def save(name, x, peak=0.85):
    x = fade(norm(x, peak))
    with wave.open(f'{OUT}/{name}.wav', 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((x * 32767).astype('<i2').tobytes())
    print(f'{name:14s} {len(x) / SR:5.2f}s')

# ---------------------------------------------------------------- voice (formant) synthesis
def voice(dur, f0a, f0b, f1, f2, f3, vib=0.03, vibhz=5.0, breath=0.12, nharm=40, shape=1.0):
    n = int(SR * dur); t = np.arange(n) / SR
    f0 = f0a * (f0b / f0a) ** (t / dur)
    f0 = f0 * (1 + vib * np.sin(2 * np.pi * vibhz * t) + 0.015 * lp(noise(n), 30, 1) * 10)
    phase = np.cumsum(f0) / SR * 2 * np.pi
    f1 = np.broadcast_to(f1, (n,)) if np.isscalar(f1) else np.interp(t, np.linspace(0, dur, len(f1)), f1)
    f2 = np.broadcast_to(f2, (n,)) if np.isscalar(f2) else np.interp(t, np.linspace(0, dur, len(f2)), f2)
    out = np.zeros(n)
    for k in range(1, nharm + 1):
        fk = k * f0
        g = (np.exp(-((fk - f1) / (0.5 * f1)) ** 2) * 1.0 + np.exp(-((fk - f2) / (0.5 * f2 * 0.6)) ** 2) * 0.55
             + np.exp(-((fk - f3) / 700.0) ** 2) * 0.2 + 0.05) / (k ** shape)
        g = g * (fk < SR / 2.2)
        out += g * np.sin(k * phase)
    out += breath * lp(noise(n), 1800) * (np.max(np.abs(out)) * 0.6)
    return out

def groan(dur, f0a, f0b, f1, f2, atk=0.12, rel=0.35, seed=0):
    x = voice(dur, f0a, f0b, f1, f2, 2500, vib=0.035, breath=0.18)
    x = lp(x, 2200)
    return x * env_ar(len(x), atk, rel) * (0.8 + 0.2 * np.sin(2 * np.pi * 2.3 * t_of(dur) + seed))

# normal zombie groans: low, moaning "uuuhhh / aaahh"
save('groan1', groan(1.25, 105, 72, [450, 650, 500], [900, 1100, 800]))
save('groan2', groan(1.00, 120, 85, [600, 500, 420], [1200, 950, 780], atk=0.08, seed=1.3))
save('groan3', groan(1.50, 92, 60, [380, 560, 420], [850, 1000, 700], atk=0.20, rel=0.5, seed=2.1))

# rage zombie: lower, harsher, tremolo + distortion
def growl(dur, f0a, f0b, trem, seed):
    x = voice(dur, f0a, f0b, [520, 700, 560], [950, 1250, 900], 2400, vib=0.06, vibhz=7, breath=0.35, nharm=50, shape=0.8)
    t = t_of(dur)
    x = x * (0.55 + 0.45 * np.sin(2 * np.pi * trem * t + seed))
    x = np.tanh(3.0 * norm(x, 1.0))
    x = lp(x, 3000) * env_ar(len(x), 0.05, 0.3)
    return x
save('growl1', growl(1.15, 62, 42, 26, 0.0))
save('growl2', growl(0.90, 70, 48, 32, 1.7))

# attack snarl (short, rising, aggressive)
x = voice(0.55, 75, 150, [700, 950], [1300, 1700], 2600, vib=0.08, vibhz=9, breath=0.5, nharm=50, shape=0.7)
x = np.tanh(3.5 * norm(x, 1.0)) * (0.6 + 0.4 * np.sin(2 * np.pi * 38 * t_of(0.55)))
save('snarl', lp(x, 3500) * env_ar(len(x), 0.03, 0.22))

# ---------------------------------------------------------------- gunshots
def shot(dur, tau, thump_f, thump_len, lp_fc, echo=0.0, crack=0.0, body=1.0):
    n = int(SR * dur); t = np.arange(n) / SR
    nz = lp(noise(n), lp_fc) * np.exp(-t / tau)
    if crack: nz += crack * hp(noise(n), 3500) * np.exp(-t / (tau * 0.25))
    f = thump_f * np.exp(-t / (thump_len * 0.6)) + 35
    th = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / thump_len) * body
    x = nz + th
    if echo:
        d = int(SR * 0.035)
        x[d:] += echo * x[:-d]
        x[2 * d:] += echo * 0.5 * x[:-2 * d]
    return x
save('shot_pistol', shot(0.35, 0.05, 170, 0.09, 7000, echo=0.25, crack=0.7, body=1.0))
save('shot_rifle',  shot(0.45, 0.07, 140, 0.12, 6000, echo=0.30, crack=0.9, body=1.3))
save('shot_shotgun', shot(0.70, 0.13, 110, 0.20, 4500, echo=0.35, crack=0.5, body=2.0))
save('shot_smg',    shot(0.22, 0.035, 200, 0.06, 8000, echo=0.15, crack=0.8, body=0.8))

# ---------------------------------------------------------------- hits, deaths, hurt
n = int(SR * 0.20); t = np.arange(n) / SR
hit = lp(noise(n), 900) * np.exp(-t / 0.04) + 0.9 * np.sin(2 * np.pi * 95 * np.exp(-t / 0.1) * t * 1.2) * np.exp(-t / 0.06)
save('hit', hit)

zd = voice(1.1, 120, 38, [600, 450, 300], [1100, 800, 600], 2300, vib=0.05, breath=0.3)
zd = lp(zd, 2000) * env_ar(len(zd), 0.02, 0.6)
zd = zd + 0.15 * lp(noise(len(zd)), 500) * np.exp(-t_of(1.1) / 0.5)
save('zombie_die', zd)

hurt = voice(0.30, 200, 125, [650, 520], [1150, 950], 2500, vib=0.02, breath=0.25)
save('hurt', lp(hurt, 2800) * env_ar(len(hurt), 0.01, 0.12))

# ---------------------------------------------------------------- pickups, ui, exit, heartbeat
def tone(f, d, vol=1.0, dec=0.07):
    t = t_of(d); return vol * (np.sin(2 * np.pi * f * t) + 0.25 * np.sin(4 * np.pi * f * t)) * np.exp(-t / dec)
def seq(notes, gap, d=0.25, dec=0.07):
    out = np.zeros(int(SR * (gap * (len(notes) - 1) + d)))
    for i, f in enumerate(notes):
        s = int(SR * gap * i); w = tone(f, d, dec=dec); out[s:s + len(w)] += w
    return out
save('pickup', seq([660, 990], 0.07, 0.18, 0.05))
save('heal', seq([523, 659, 784], 0.08, 0.35, 0.12))
n = int(SR * 0.22); t = np.arange(n) / SR
click = lambda: hp(noise(int(SR * 0.04)), 1500) * np.exp(-t_of(0.04) / 0.008)
sw = np.zeros(n); c = click(); sw[:len(c)] += c; sw[int(SR * 0.09):int(SR * 0.09) + len(c)] += 0.8 * c
sw += 0.5 * np.sin(2 * np.pi * 1800 * t) * np.exp(-t / 0.03) + 0.4 * np.sin(2 * np.pi * 220 * t) * np.exp(-t / 0.05)
save('weapon_swap', sw)
save('exit', seq([392, 523, 659, 784], 0.11, 0.45, 0.16))
save('ui_click', tone(820, 0.07, dec=0.02))
n = int(SR * 0.75); t = np.arange(n) / SR
hb = np.zeros(n)
for start in (0.0, 0.27):
    s = int(start * SR); tt = t_of(0.2)
    thump = np.sin(2 * np.pi * (70 * np.exp(-tt / 0.08) + 35) * tt) * np.exp(-tt / 0.07)
    hb[s:s + len(thump)] += thump * (1.0 if start == 0 else 0.75)
save('heartbeat', hb)
