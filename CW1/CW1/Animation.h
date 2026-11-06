#pragma once
#include "Timer.h"

class Animation {
	Timer timer;
	int frameCount,offset;

public:
	Animation() : timer(0), frameCount(0), offset(0) {}
	Animation(int frameCount, float length) : frameCount(frameCount), timer(length), offset(0) {}
	Animation(int frameCount, float length, int offset) : frameCount(frameCount), timer(length), offset(offset) {}

	float getLength()const { return timer.getLength(); }
	int currentFrame()const { return (static_cast<int>(timer.getTime() / timer.getLength() * frameCount) + offset)% frameCount; }

	void step(float deltaTime) { timer.step(deltaTime); }
	bool isDone() const { return timer.isTimeout(); }
};