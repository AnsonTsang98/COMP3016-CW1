#pragma once
#include "SDL3/SDL.h"
#include "Animation.h"

#include <vector>

using namespace std;

enum class PlayerState {
	idle, walking, dead
};

enum class BulletState {
	moving, colliding, inactive
};

enum class ObjectType {
	player, level, enemy, bullet, item
};

enum class EnemyState {
	idle, roaming, attack, damaged, dead
};

enum class ItemType {
	speedup, heal, weapon, flashlight, SIZE
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

struct Weapon {
	float damage, cooldown, spread;
	int bullet,anim;
	Timer pickUpCooldown;
	Weapon() :pickUpCooldown(1.0f){
		damage = cooldown = spread = bullet = anim = -1;
	}
};

struct Pistol : Weapon {
	Pistol():Weapon(){
		damage = 10;
		cooldown = 0.4f;
		spread = 10;
		bullet = 1;
		anim = 3;
	}
};

struct Rifle : Weapon {
	Rifle() :Weapon() {
		damage = 25;
		cooldown = 0.2f;
		spread = 10;
		bullet = 1;
		anim = 4;
	}
};

struct Submachine : Weapon {
	Submachine() :Weapon() {
		damage = 15;
		cooldown = 0.1f;
		spread = 10;
		bullet = 1;
		anim = 6;
	}
};

struct Shotgun : Weapon {
	Shotgun() :Weapon() {
		damage = 8;
		cooldown = 0.8f;
		spread = 40;
		bullet = 8;
		anim = 5;
	}
};

struct Zombie {
	float hp, damage, speed;
	int score;
};

struct Normal : Zombie {
	Normal() :Zombie() {
		damage = 10;
		hp = 150;
		speed = 50;
		score = 1;
	}
};

struct Rage : Zombie {
	Rage() :Zombie() {
		damage = 20;
		hp = 240;
		speed = 80;
		score = 5;
	}
};

struct ItemData {
	float value;
	ItemType type;
	Weapon* gun;
	bool active;
	ItemData():value(0),gun(nullptr),active(true) {
		type = (ItemType)SDL_rand((Sint32)ItemType::SIZE);
		switch (type) {
			case ItemType::speedup: {
				value = 5;
				break;
			}
			case ItemType::heal: {
				value = 50;
				break;
			}
			case ItemType::flashlight: {
				value = 1;
				break;
			}
			case ItemType::weapon: {
				value = (SDL_rand(4) + 1) * -1;
				break;
			}
		}
	}
};

struct PlayerData {
	PlayerState state;
	Timer weaponTimer;
	float playerSpeed = 100;
	int playerVision = 4;
	float playerFov = 45.0f;
	float playerMaxHP = 100;
	float playerHP;
	float extraCooldown = 0.2f;
	int lastAnim = 0;
	Weapon* gun;
	PlayerData() : gun(nullptr),weaponTimer(0.3f),playerHP(playerMaxHP) {
		state = PlayerState::idle;
	}
};

struct LevelData {
	LevelData() {
	}
};

struct EnemyData {
	EnemyState state;
	Timer damagedTimer, stateTimer, cooldownTimer;
	float idleTime, roamingTime, attackTime;
	Zombie* z;
	EnemyData() : z(nullptr),state(EnemyState::idle), damagedTimer(0.5f), stateTimer(5.0f), cooldownTimer(0.5f),
		idleTime(5.0f), roamingTime(2.3f), attackTime(4.0f) {
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
	ItemData item;
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