# Automated tests

These tests need no graphics card or sound card. Run them from this `tests` folder (the game headers are in the folder above).

| Test | What it checks | Command |
|---|---|---|
| `test_upgrade_data.cpp` | Buying (cost, not enough score, max level), JSON save/load round trip, missing save file, damaged / out-of-range / garbage JSON, effect values | `g++ -std=c++14 -Wall -Wextra -I.. test_upgrade_data.cpp -o test_upgrade_data && ./test_upgrade_data` |
| `test_audio_antispam.cpp` | The sound system ignores repeated requests (hurt sound every frame, 8 shotgun pellets, 3 zombie voices at once, too many gunshots) | `g++ -std=c++14 -Wall -Wextra -Isdl_stub -I.. test_audio_antispam.cpp -o test_audio && ./test_audio` |
| `validate_map.py` | The 30 x 50 level in `CW1.cpp`: size, solid border, one player, one exit, ground under every non-wall tile, and every item / zombie / the exit can be reached from the player | `python3 validate_map.py ../CW1.cpp` |

`sdl_stub/` is a tiny fake of the SDL3 functions that `AudioSystem.h` uses, with a controllable clock, so the audio rules can be tested without a sound device.

Expected output: `all checks passed` (twice) and `validate_map: ok ...`.

The gameplay test cases that need a human (playing the game) are in the report, section "Exception handling and test cases".
