# Playtesting

> **Read me first (student):** nothing in this file is filled in with invented results. Run a short playtest, then replace every **TODO**. 3 – 5 testers for 10 minutes each is enough. Anonymise names (Tester A, B, C). Keep the raw evidence (screenshots of the form, photos of the session, notes) in `docs/playtesting/`.

## 1. Plan

| Item | Value |
|---|---|
| Build tested | **TODO** commit hash / release version |
| Date(s) | **TODO** |
| Where | **TODO** (lab / home / game society) |
| Testers | **TODO** e.g. 4 peers (Tester A – D), anonymised |
| Task given | "Play 3 runs. Try to reach the exit. Spend your score in UPGRADE between runs. Think aloud." |
| What I watched for | Did they find the controls (WASD, mouse, E)? Did they understand the exit? Was the fog too dark? Was the difficulty fair? Any crash or bug? Was the sound annoying? |

## 2. Questionnaire (copy into a Google / Microsoft Form, or print)

Ask the tester to rate 1 (strongly disagree) to 5 (strongly agree):

1. I understood the controls without help.
2. I understood what I had to do (find the exit and press E).
3. The difficulty was fair.
4. The dark / fog of war made the game more tense (not just frustrating).
5. The upgrade page was easy to understand and the upgrades felt useful.
6. The zombie sounds helped me react.
7. The sound effects were not annoying or repetitive.
8. The game ran smoothly (no lag).
9. I would play another run.

Open questions: What was the most confusing moment? What was the best moment? What one thing would you change? Did anything break?

## 3. Results

| Question | A | B | C | D | Average |
|---|---|---|---|---|---|
| 1 Controls | | | | | |
| 2 Goal | | | | | |
| 3 Difficulty | | | | | |
| 4 Fog | | | | | |
| 5 Upgrades | | | | | |
| 6 Zombie sound helps | | | | | |
| 7 Sound not annoying | | | | | |
| 8 Smooth | | | | | |
| 9 Replay | | | | | |

**TODO**: paste comments (anonymised) and bugs found.

## 4. Feedback to changes (this is what earns the marks)

| Feedback received (who) | Change made | Commit / evidence |
|---|---|---|
| **TODO** e.g. "Zombies keep groaning, it is too much" (Tester B) | Added anti-spam rules in `AudioSystem.h` | commit `...` |
| **TODO** | | |

Examples of issues that already came out of the student's own play sessions during development (these are real, from the AI conversation):
- "the sound will bugged": same sound repeating too often / all at once. Fixed with anti-spam rules in `AudioSystem.h`.
- "the zombie will collide with their dead body". Fixed in `collisionResponse()`.
- "it's very slow now". Fixed with the fog texture and collision filtering.
- Null pointer crash on the upgrade data. Fixed.

## 5. Reflection (write this in your own words)

- **Who took part:** **TODO**
- **What was tested:** **TODO**
- **Feedback received:** **TODO**
- **Changes implemented:** **TODO**
- **Remaining limitations:** **TODO** (e.g. only a few testers, no controller support, no music, all testers were classmates)
