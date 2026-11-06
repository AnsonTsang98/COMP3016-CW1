#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

//The four upgrades
enum UpgradeType {
	UP_MAXHP = 0,
	UP_SPEED,
	UP_VISION,
	UP_FIREARMS,
	UP_COUNT
};

//Everything the player keeps between runs: the score (currency) and the level of each upgrade.
//All tuning numbers live in the small static functions below, so you can balance the game in one place.
struct UpgradeData {
	int score;
	int level[UP_COUNT];

	UpgradeData() { reset(); }

	void reset() {
		score = 0;
		for (int i = 0; i < UP_COUNT; i++)
			level[i] = 0;
	}

	// ------------------------------------------------------------ tuning
	static int maxLevel(int type) {
		switch (type) {
			case UP_VISION: return 3;
			default:        return 5;
		}
	}

	//cost of the NEXT level: base * (level + 1)
	int cost(int type) const {
		static const int base[UP_COUNT] = { 10, 10, 15, 20 };	//const array of ints: fine in C++14
		return base[type] * (level[type] + 1);
	}

	static const char* name(int type) {
		switch (type) {
			case UP_MAXHP:    return "MAX HP";
			case UP_SPEED:    return "SPEED";
			case UP_VISION:   return "VISION";
			default:          return "FIREARMS";
		}
	}

	// ------------------------------------------------------------ effects (what the game reads)
	//level -> value helpers take a level so the menu can also preview the next level
	static float maxHpAt(int lv)       { return 100.0f + 20.0f * lv; }		//base 100, +20 per level
	static float speedMultAt(int lv)   { return 1.0f + 0.10f * lv; }			//+10% per level
	static int   visionBonusAt(int lv) { return lv; }							//+1 vision radius per level
	static float accuracyAt(int lv)  { return 2.0f * lv; }			//+2 accuracy per level
	static float cooldownMultAt(int lv){ return 0.04f * lv; }			//-0.04 weapon cooldown per level

	float maxHp() const        { return maxHpAt(level[UP_MAXHP]); }
	float speedMult() const    { return speedMultAt(level[UP_SPEED]); }
	int   visionBonus() const  { return visionBonusAt(level[UP_VISION]); }
	float accuracy() const   { return accuracyAt(level[UP_FIREARMS]); }
	float cooldownMult() const { return cooldownMultAt(level[UP_FIREARMS]); }

	// ------------------------------------------------------------ shopping
	bool isMax(int type) const { return level[type] >= maxLevel(type); }
	bool canBuy(int type) const { return !isMax(type) && score >= cost(type); }

	bool buy(int type) {
		if (!canBuy(type))
			return false;
		score -= cost(type);
		level[type]++;
		return true;
	}

	// ------------------------------------------------------------ JSON save / load
	//File looks like:
	//{
	//  "score": 47,
	//  "maxHp": 2,
	//  "speed": 1,
	//  "vision": 0,
	//  "firearms": 3
	//}
	//Small hand-written reader/writer so there is no extra library to install. If you already use
	//nlohmann/json, replace toJson()/fromJson() with  j["score"] = score; ...  and  j.value("score", 0); ...
	static const char* key(int type) {
		switch (type) {
			case UP_MAXHP:  return "maxHp";
			case UP_SPEED:  return "speed";
			case UP_VISION: return "vision";
			default:        return "firearms";
		}
	}

	std::string toJson() const {
		std::ostringstream o;
		o << "{\n  \"score\": " << score;
		for (int i = 0; i < UP_COUNT; i++)
			o << ",\n  \"" << key(i) << "\": " << level[i];
		o << "\n}\n";
		return o.str();
	}

	//reads an integer value after "name":  -> false if the key is missing
	static bool readInt(const std::string& json, const char* name, int& out) {
		std::string pattern = std::string("\"") + name + "\"";
		size_t pos = json.find(pattern);
		if (pos == std::string::npos)
			return false;
		pos = json.find(':', pos + pattern.size());
		if (pos == std::string::npos)
			return false;
		const char* start = json.c_str() + pos + 1;
		char* end = nullptr;
		long v = std::strtol(start, &end, 10);
		if (end == start)
			return false;
		out = (int)v;
		return true;
	}

	//missing / damaged values keep their defaults; numbers are clamped so a edited save file can not break the game
	bool fromJson(const std::string& json) {
		UpgradeData d;
		bool ok = readInt(json, "score", d.score);
		for (int i = 0; i < UP_COUNT; i++)
			ok = readInt(json, key(i), d.level[i]) && ok;
		if (d.score < 0)
			d.score = 0;
		for (int i = 0; i < UP_COUNT; i++) {
			if (d.level[i] < 0)
				d.level[i] = 0;
			if (d.level[i] > maxLevel(i))
				d.level[i] = maxLevel(i);
		}
		*this = d;
		return ok;
	}

	bool save(const char* path = "save.json") const {
		std::ofstream f(path, std::ios::trunc);
		if (!f)
			return false;
		f << toJson();
		return (bool)f;
	}

	//no file yet (first launch) -> returns false and keeps the defaults
	bool load(const char* path = "save.json") {
		std::ifstream f(path);
		if (!f) {
			reset();
			return false;
		}
		std::stringstream ss;
		ss << f.rdbuf();
		return fromJson(ss.str());
	}
};
