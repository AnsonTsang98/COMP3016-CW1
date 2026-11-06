# AI Development Log

Project: **Escape Anything** (COMP3016 Coursework 1). Level of AI use declared: **Partnered Work** (see the signed declaration form).

The full conversation is exported in [AI_CHAT_LOG.md](AI_CHAT_LOG.md). This file is the summary the brief asks for: tools used, categories of use, example prompts, the verification process and repository evidence.

## 1. AI tools used

| Tool | Version / how it was used | Used for |
|---|---|---|
| **Claude** (Anthropic), model *Claude Sonnet 5.5*, in the Claude app / Claude Code environment (it can run Python and C++ in a sandbox and send files) | One long conversation, 5 – 6 November 2026 (27 student messages) | Assets (sprites, animations, UI, tiles, sound effects), level design help, new C++ classes, debugging, optimisation suggestions, tests, documentation |

No other AI tool was used for this project's assets or code in the conversation covered by this log. **TODO (student): if you used any other AI tool (ChatGPT, Copilot, Gemini, ...) for any part of the game, add it here and in the declaration form.**

## 2. What the student wrote and what the AI produced

Be clear about ownership (the brief asks for it):

| Part | Written by | Notes |
|---|---|---|
| Game architecture and core engine: `GameObject.h`, `Timer.h`, `Animation.h`, most of `CW1.cpp` (game loop, player / enemy / bullet update, collision response, fog of war, resource loading, `createTiles`) | **Student** | Already existed when the AI was first shown the code (message 14). The student pasted it into the conversation; the AI only reviewed it. |
| Pixel-art BMP assets (items, animations, characters, bleed overlay, menu, upgrade page, ground, wall, exit) | **AI-generated** by Python scripts (`tools/`), directed and iterated by the student | Student defined the specification ("item spec"), chose what to keep, asked for changes |
| Sound effects (19 WAV files) | **AI-generated** by a Python synthesiser (`tools/make_sounds.py`) | Synthetic sounds; the student listened and reported problems |
| `MainMenu.h`, `UpgradeMenu.h`, `UpgradeData.h`, `AudioSystem.h` | **AI-written first version**, then **modified by the student** | e.g. the student changed the Firearms upgrade from "damage" to "accuracy + cooldown", changed enemy codes in the map, wired everything into `CW1.cpp` |
| Edits inside `CW1.cpp` (menu loop, exit / run end, restart, performance changes, sound calls) | **AI wrote edit lists** (text files with the code to paste); the **student applied them by hand** | The student chose to apply these by hand (message 22) |
| Level layout (30 x 50 map) | **AI-generated** by `tools/make_map.py` (room generator + reachability check) | Student edited it by hand afterwards (enemy placement) |
| This report / README | AI-assisted drafting from the real project facts; student edits | "Readme report crafting" is an allowed use in the brief |

## 3. Categories of use

| Category (from the brief) | What was done | Main output |
|---|---|---|
| Sprite creation | 6 item sprites in a fixed spec (32x32, 4 – 8 colours, white = transparent key); 11 top-down characters (player with 4 weapons + dead, normal zombie x3, rage zombie x3) | `Assets/*.bmp` |
| Animation creation | 8-frame item float / shine animations; 8-frame walk cycles; idle-to-dead animations; 24-frame blood overlay (4 x 6 sheet) | `*_anim.bmp`, `*_walk.bmp`, `bleed_anim_sheet.bmp` |
| UI assets | Main menu (background, title, buttons), upgrade page (title, rows, icons, pips, + / MAX button) | `menu_*.bmp`, `upgrade_*.bmp` |
| Texture generation | Ground and wall tiles; animated exit hatch | `ground3.bmp`, `wall2.bmp`, `exit_anim.bmp` |
| Sound generation | 19 sound effects (zombie groans / growls / snarl, 4 gunshots, hit, death, hurt, pickup, heal, swap, exit, UI, heartbeat) | `Assets/Sounds/*.wav` |
| Code generation | Menu, upgrade page, upgrade data with JSON, audio system, exit / run-restart logic | `MainMenu.h`, `UpgradeMenu.h`, `UpgradeData.h`, `AudioSystem.h` |
| Debugging | Bleed overlay not smooth (operator precedence bug), null pointer crash (`gs.upgrades`), zombies colliding with dead bodies, repeated sounds | see section 5 |
| Optimisation | Fog of war: 1.5 million draw calls per frame replaced by one texture, visible-cell list, collision filtering, off-screen culling | `CW1_performance_changes.txt` |
| Level design support | 30 x 50 level generator with solid border and reachability check | `tools/make_map.py` |
| Gameplay balancing | Upgrade table (costs, per-level bonuses, max levels), zombie hearing range and groan timing | `UpgradeData.h`, `AudioSystem.h` |
| Programming testing | Unit tests for upgrade data and the audio rules, map validator | `tests/` |
| Documentation / readme report crafting | This report, this log, the playtest template, video script | `README.md`, `docs/` |

Not used: narration, dialogue, music, model generation.

## 4. Example prompts

Real prompts from the conversation (typos are original). The number is the message number in [AI_CHAT_LOG.md](AI_CHAT_LOG.md).

| # | Prompt (shortened where marked) | What the AI produced |
|---|---|---|
| 2 | "...This is the specification I need: 32x32 pixel grid, max 4–8 colours, use RBGA 255,255,255,255 as a transparent background colour, so don't use this colour for the sprite, use a hard integer grid with no anti-aliasing or soft gradients. With those specification, please generate some items for me, include: med kit, flashlight, pistol (like glock-17), assault rifle (like HK416), submachine gun (like UMP), shotgun (like M870)" | Item BMPs (a Python script drew them; the student later named this the "item spec") |
| 4 | "Now I need a bleeding screen effect to tell player how many hp they left, use the 'item spec'... but change a little bit for only this time: 320 width x 640 height pixel grid, avoid screen centre as possible (I don't want player too difficult to watch the playable area)" | First blood-overlay design (4 design rounds followed, messages 5 – 10) |
| 11 | "...create some characters. I need main character, normal zombie, and rage zombie. The game I make mainly using black, grey colours, please make main character mainly use black colour, normal zombie mainly use dark green colour, and rage zombie mainly use dark red colour. Beside, my game is top-down view... main character using the pistol, rifle, shotgun, and SMG... dead state..." | 11 top-down character BMPs |
| 14 | "About the bleeding screen overlay that you generate for me before, it can't render smoothly, this is my code, can you help me to check it?" (attached `CW1.cpp`, `Animation.h`, `Timer.h`) | Found the bug in the student's source-rectangle maths (section 5, item 5) |
| 15 | "Now I am going to create a main menu, can you help me to generate a sample with code, and relative image, here is my code for your reference." | `MainMenu.h` + menu images + integration notes |
| 16 | "...I want a upgrade button as well... upgrade some status of the player, such as max hp, speed, vision, and firearms proficiency. I will create a new struct to save the upgrade, and use JSON to do the save load system. Using the score from killing zombie in game to upgrade. Please generate a sample as well, just like the main menu." | `UpgradeData.h`, `UpgradeMenu.h`, images, edit list |
| 17 | "Exception thrown: read access violation. this was nullptr. in the line 67 of upgradedata.h" | Diagnosis: a null `UpgradeData*` in `GameState` |
| 20 | "help me generate a new game map, this time I will set the map rows to 30 and map cols to 50. Also generate new ground and wall image please" | Map generator, tiles, preview |
| 22 | "can you use txt to tell me which part need to edit? Like last time, I would like to edit by myself." | The student's choice: AI gives edit lists; the student applies them |
| 23 | "can you help me add sound effect inside the game? At least have sound effect when zombie is nearby." | `AudioSystem.h`, 19 sounds, edit list |
| 24 | "it's very slow now, any things can do to reduce the latency?" | Performance analysis and edit list |
| 25 – 26 | "the sound will bugged and the zombie will collide with their dead body" / "I think is the same sound repeating too often or all at once." | Dead-body collision fix, sound anti-spam rules |

## 5. Verification process: what the AI produced, how it was reviewed, issues found, changes made

| # | AI output | How it was checked | Issue found | Modification / improvement |
|---|---|---|---|---|
| 1 | First reply to message 1 | Student read it | It made 16 x 16 PNG sprites and SDL2 / SDL_image code, but the project uses SDL3 and BMP | Student wrote an exact specification (message 2); all assets were regenerated as 32x32 indexed BMPs. The AI also had to learn that the student's loader treats **palette index 0** as the transparent colour, so the generator now always writes white at index 0. |
| 2 | Item sprites | Student viewed previews; script has asserts (no sprite pixel may use the key colour, image size, palette size 8 or less) | Early gun sprites overflowed the 32 x 32 grid (assert failed) | Gun parts were shortened |
| 3 | Characters | Student viewed previews and asked for changes (messages 12 – 13) | SMG too big, face hidden by rifle / shotgun, no cap, feet invisible in walk cycle | Resized, face made visible, cap added, stride widened; static sprites were byte-compared with the earlier version to confirm only intended changes |
| 4 | Blood overlay | Student reviewed 6 rounds (messages 4 – 10) | Wrong orientation (320 x 640 instead of 640 x 320), bottom edge not wanted, drips too fast / too many on the sides, droplets too small, drips could not reach the bottom in 16 frames | Re-oriented, redesigned, drip speeds separated from droplet speed, frames increased from 16 to 24; the script asserts that no drip enters the screen centre |
| 5 | Review of the student's overlay code | Student ran the game | The source rectangle used `state.logW * frame % 4` (multiplication before modulo) and divided the row by 6 although the sheet has 4 columns x 6 rows | Student fixed both lines (`% 4` and `/ 4`); the AI also noted the true performance cost was the fog loops |
| 6 | Menu / upgrade code | Compiled with a stub SDL3 header (g++ and clang++), a small program called `handleEvent` with fake key and mouse events | Only warnings about member order in the student's `Animation.h` | Student integrated by hand |
| 7 | `UpgradeData` JSON | C++ round-trip test, later saved as `tests/test_upgrade_data.cpp` | None in the AI version; the test covers missing / damaged / out-of-range files | Values are clamped on load |
| 8 | Null pointer crash (message 17) | Student's debugger showed line 67 | `gs.upgrades` was null when used (pointer set late / GameState rebuilt) | AI proposed null-safe accessors and setting the pointer after every `GameState` creation; the student fixed it |
| 9 | Student changed the AI's Firearms upgrade | Review of the student's version of the headers | The student turned "damage" into "accuracy" and "cooldown" (independent design change) | Shows ownership: the design is the student's. **Known issue:** the `snprintf` format for the Firearms row in `UpgradeMenu.h` was edited to `%d%%` with float arguments; it must print `(int)` values (see section 7) |
| 10 | Map | `make_map.py` has asserts; later `tests/validate_map.py` | Needed an exit tile (code 7) and rage zombies (code 2) after the student changed the codes | Regenerated; checks: solid border, one player, one exit, everything reachable |
| 11 | Exit / run restart | The student asked for the exact rules (exit with E, only exit or death returns to the menu, everything restarts except upgrades) | The new exit object had to be non-solid for zombies, ignored by bullets and drawn without rotation | Edits to `collisionResponse`, `drawObject`, `createTiles`; `restartGame()` rebuilds `GameState` |
| 12 | Sounds | The AI can not listen; it checked format (16-bit mono 44.1 kHz), peak level and no NaN; the student listened in the game | Sounds repeated too often / all at once (message 26); zombies collided with dead bodies (message 25) | Anti-spam rules in `AudioSystem.h` (gaps, max copies, one zombie voice at a time), tested with a fake clock (`tests/test_audio_antispam.cpp`); corpses no longer collide |
| 13 | Performance | Counts of work per frame and a micro-benchmark: full-map reset 3.5 ms (0.6 ms optimised) against 0.07 ms with a visible list. Not measured in the real game by the AI | Fog loop made up to 1.5 million draw calls per frame; every wall tested collision against every object (about 142,000 checks) | One streaming fog texture, visible-cell list, only moving objects test collisions within 2 tiles, off-screen culling; build in Release |

Limits of the AI checks (honest): the AI never ran the real game or the Visual Studio build. It compiled the new headers against a stub of SDL3 and tested logic with small programs. All integration, compiling, playing and listening was done by the student.

## 6. Repository evidence

The Git history should show the refinement after AI output. Useful commits to point to (replace the hashes with yours):

| What | Commit message / hash | Shows |
|---|---|---|
| **TODO** | e.g. `Fix bleed overlay frame indexing` — `abc1234` | Student fixed the AI-diagnosed bug |
| **TODO** | e.g. `Change firearms upgrade to accuracy and cooldown` — `...` | Student changed the AI's design |
| **TODO** | e.g. `Corpses no longer block zombies` — `...` | Bug found by playing, then fixed |
| **TODO** | e.g. `Fog of war: single texture, visible-cell list` — `...` | Performance refinement |

Also attach: GitHub Insights contribution graph, milestone commit screenshots, resolved issues (see `docs/images/`, **TODO**).

## 7. Known issues left after the AI-assisted work

- `UpgradeMenu.h`, `effectText()`, Firearms row: the format string `"accuracy +%d% (MAX)"` contains an invalid `% (` and passes `float` to `%d`. Use: `std::snprintf(out, size, max ? "ACCURACY +%d (MAX)" : "ACCURACY +%d > +%d", (int)UpgradeData::accuracyAt(lv), (int)UpgradeData::accuracyAt(nx));` **(fix this before recording the video if you have not already)**.
- The debug text still divides the bleed frame by 6 for display (`currentFrame() / 6`), it should be `/ 4` (debug display only).
- `loadTexture()` does not check that the BMP file was found.
- `CW1.cpp` is one large file; splitting it (rendering, collision, fog, level loading) is future work.
