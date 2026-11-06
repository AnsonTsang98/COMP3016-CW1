// Test of the anti-spam rules in AudioSystem.h. SDL3 is replaced by a tiny stub (sdl_stub/SDL3/SDL.h) with a fake
// clock, so the test needs no sound card. Build (from the tests folder, AudioSystem.h is in the folder above):
//   g++ -std=c++14 -Wall -Wextra -Isdl_stub -I.. test_audio_antispam.cpp -o test_audio && ./test_audio
#include "AudioSystem.h"
#include <cassert>
#include <cstdio>

int main() {
	AudioSystem a;
	assert(a.init());                 // the stub returns 1.0 s long sounds
	int& puts_ = putCalls();          // how many sounds really started

	// The hurt sound is requested every frame (16 ms) while a zombie touches the player: only one may start.
	puts_ = 0;
	for (int i = 0; i < 60; i++) { a.play(SND_HURT); fakeTicks() += 16; }
	assert(puts_ == 1);
	fakeTicks() += 3000;

	// Eight shotgun pellets hit in one frame: one hit sound.
	puts_ = 0;
	for (int i = 0; i < 8; i++) a.play(SND_HIT);
	assert(puts_ == 1);
	fakeTicks() += 3000;

	// Three different zombie voices requested together: only one plays; the next one waits for the gap.
	puts_ = 0;
	a.play(SND_GROAN1); a.play(SND_GROWL1); a.play(SND_SNARL);
	assert(puts_ == 1);
	fakeTicks() += 1100; a.play(SND_GROAN2); assert(puts_ == 1);   // too soon / still playing
	fakeTicks() += 600;  a.play(SND_GROAN2); assert(puts_ == 2);   // allowed after the gap
	fakeTicks() += 3000;

	// At most 3 gunshots overlap.
	puts_ = 0;
	for (int i = 0; i < 6; i++) { a.play(SND_SHOT_SMG); fakeTicks() += 150; }
	assert(puts_ == 3);
	fakeTicks() += 3000;

	// Different sounds do not block each other.
	puts_ = 0;
	a.play(SND_PICKUP); a.play(SND_HEAL); a.play(SND_SWAP); a.play(SND_UI_CLICK); a.play(SND_HEARTBEAT); a.play(SND_ZOMBIE_DIE);
	assert(puts_ == 6);

	// Invalid ids are ignored instead of crashing.
	a.play(-1); a.play(SND_COUNT + 5);

	std::puts("test_audio_antispam: all checks passed");
	return 0;
}
