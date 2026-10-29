#pragma once
#include "SDL3/SDL.h"
#include "Animation.h"

#include <vector>

using namespace std;

enum class PlayerState {
	idle, walking
};

enum class ObjectType {
	player, level, enemy
};

struct Vec2 {
	float x, y;
	Vec2() :x(0), y(0) {}
	Vec2(float posX, float posY){x = posX; y = posY; }
};

struct PlayerData {
	PlayerState state;
	float playerSpeed = 75;
	int playerVision = 4;
	float playerFov = 45.0f;
	PlayerData() {
		state = PlayerState::idle;
	}
};

struct LevelData {
	LevelData() {
	}
};

struct EnemyData {
	EnemyData() {
	}
};

union ObjectData {
	PlayerData player;
	LevelData level;
	EnemyData enemy;
};

struct GameObject {
	ObjectType type;
	ObjectData data;
	Vec2 position, velocity, acceleration;
	float direction;
	vector<Animation> animations;
	int currentAnimation;
	SDL_Texture* texture;

	GameObject() : data{} {
		type = ObjectType::level;
		data.level = LevelData();
		direction = 1;
		//position = velocity = acceleration = Vec2();
		currentAnimation = -1;
		texture = nullptr;
	}
};