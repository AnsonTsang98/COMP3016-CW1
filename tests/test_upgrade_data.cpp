// Unit test for UpgradeData (score, buying, JSON save / load, damaged save files).
// Build (from the tests folder, UpgradeData.h is in the folder above):
//   g++ -std=c++14 -Wall -Wextra -I.. test_upgrade_data.cpp -o test_upgrade_data && ./test_upgrade_data
#include "UpgradeData.h"
#include <cassert>
#include <cstdio>

int main() {
	// --- buying -----------------------------------------------------------------------------------------------
	UpgradeData a;
	a.score = 100;
	assert(a.buy(UP_MAXHP));                         // level 0 -> 1 costs 10
	assert(a.score == 90 && a.level[UP_MAXHP] == 1);
	assert(a.cost(UP_MAXHP) == 20);                  // next level costs base * (level + 1)
	a.score = 5;
	assert(!a.buy(UP_FIREARMS));                     // not enough score: nothing changes
	assert(a.score == 5 && a.level[UP_FIREARMS] == 0);
	a.level[UP_VISION] = UpgradeData::maxLevel(UP_VISION);
	a.score = 9999;
	assert(a.isMax(UP_VISION) && !a.buy(UP_VISION)); // max level: can not buy more
	assert(a.score == 9999);

	// --- JSON round trip ------------------------------------------------------------------------------------------
	a.reset();
	a.score = 47; a.level[UP_MAXHP] = 2; a.level[UP_SPEED] = 1; a.level[UP_VISION] = 3; a.level[UP_FIREARMS] = 5;
	assert(a.save("test_save.json"));
	UpgradeData b;
	assert(b.load("test_save.json"));
	assert(b.score == 47 && b.level[UP_MAXHP] == 2 && b.level[UP_SPEED] == 1 && b.level[UP_VISION] == 3 && b.level[UP_FIREARMS] == 5);

	// --- edge cases: the game must not crash on a missing or damaged save file ---------------------------------------
	UpgradeData c;
	c.score = 77;
	assert(!c.load("this_file_does_not_exist.json")); // first launch: defaults, returns false
	assert(c.score == 0 && c.level[UP_MAXHP] == 0);

	UpgradeData d;
	d.fromJson("{\"score\": -5, \"maxHp\": 99, \"speed\": }");   // negative, too big, empty value
	assert(d.score == 0);                                        // clamped to 0
	assert(d.level[UP_MAXHP] == UpgradeData::maxLevel(UP_MAXHP));// clamped to the max level
	assert(d.level[UP_SPEED] == 0);                              // unreadable value -> default

	UpgradeData e;
	assert(!e.fromJson("this is not json at all"));              // garbage text: defaults
	assert(e.score == 0);

	// --- effect values used by the game --------------------------------------------------------------------------
	UpgradeData f;
	assert(f.maxHp() == 100.0f && f.speedMult() == 1.0f && f.visionBonus() == 0);
	f.level[UP_MAXHP] = 5; f.level[UP_SPEED] = 5; f.level[UP_VISION] = 3;
	assert(f.maxHp() == 200.0f && f.visionBonus() == 3);

	std::puts("test_upgrade_data: all checks passed");
	return 0;
}
