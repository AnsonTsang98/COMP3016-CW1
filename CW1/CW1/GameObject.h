#pragma once
#include "SDL3/SDL.h"
#include "Animation.h"

#include <vector>

using namespace std;

enum class PlayerState {
	idle, walking
};

enum class BulletState {
	moving, colliding, inactive
};

enum class ObjectType {
	player, level, enemy, bullet
};

enum class EnemyState {
	idle, roaming, attack, damaged, dead
};


struct Vec2 {
	float x, y;
	Vec2() :x(0), y(0) {}
	Vec2(float posX, float posY){x = posX; y = posY; }
	friend Vec2 operator*(Vec2 v1, const float f) {
		v1.x *= f;
		v1.y *= f;
		return v1;
	}
	friend Vec2 operator*( const float f, Vec2 v1) {
		return v1 * f;
	}
	friend Vec2 operator*(Vec2 v1, const Vec2 v2) {
		v1.x *= v2.x;
		v1.y *= v2.y;
		return v1;
	}
	friend Vec2 operator+(Vec2 v1,const Vec2& v2) {
		v1.x += v2.x;
		v1.y += v2.y;
		return v1;
	}
	Vec2& operator+=(const Vec2& v2) {
		*this = *this + v2;
		return *this;
	}
	Vec2& operator*=(const float f) {
		*this = *this * f;
		return *this;
	}
};

struct PlayerData {
	PlayerState state;
	Timer weaponTimer;
	float playerSpeed = 75;
	int playerVision = 4;
	float playerFov = 45.0f;
	PlayerData() : weaponTimer(0.1f){
		state = PlayerState::idle;
	}
};

struct LevelData {
	LevelData() {
	}
};

struct EnemyData {
	EnemyState state;
	Timer damagedTimer, idleTimer, roamingTimer, attackTimer, cooldownTimer;
	int healthPoints;
	EnemyData() : state(EnemyState::idle), damagedTimer(0.5f), idleTimer(5.0f), roamingTimer(2.3f), attackTimer(4.0f), cooldownTimer(0.5f) {
		healthPoints = 100;
	}
};

struct BulletData {
	BulletState state;
	BulletData() : state(BulletState::moving){
	}
};

union ObjectData {
	PlayerData player;
	LevelData level;
	EnemyData enemy;
	BulletData bullet;
};

struct GameObject {
	ObjectType type;
	ObjectData data;
	Vec2 position, velocity, direction;
	float maxSpeed, acceleration;
	vector<Animation> animations;
	int currentAnimation, spriteFrame;
	SDL_Texture* texture;
	SDL_FRect collider;
	Timer flashTimer;
	bool shouldFlash;

	GameObject() : type(ObjectType::level), data{}, collider{ 0 }, flashTimer(0.05f) {
		type = ObjectType::level;
		data.level = LevelData();
		direction.x = direction.y = -1;
		maxSpeed = acceleration = 0;
		//position = velocity = Vec2();
		currentAnimation = -1;
		texture = nullptr;
		shouldFlash = false;
		spriteFrame = 1;
	}
};