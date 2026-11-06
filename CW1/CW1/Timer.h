#pragma once

class Timer {
	float length, time;
	bool timeout;

public:
	Timer(float length) : length(length), time(0), timeout(false){}

	bool step(float deltaTime) {
		time += deltaTime;
		if (time >= length) {
			time -= length;
			timeout = true;
			return true;
		}
		return false;
	}

	bool isTimeout() const { return timeout; }
	float getTime() const { return time; }
	float getLength() const { return length; }
	void setLength(float l) { length = l; }
	void reset() { time = timeout = 0; }
};