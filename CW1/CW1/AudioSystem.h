#pragma once
#include "SDL3/SDL.h"

#include <string>

//every sound effect; the order matters for the groups (GROAN1..3, GROWL1..2, SHOT_*)
enum SoundId {
	SND_GROAN1 = 0, SND_GROAN2, SND_GROAN3,				//normal zombie nearby
	SND_GROWL1, SND_GROWL2,								//rage zombie nearby
	SND_SNARL,											//zombie attacking
	SND_SHOT_PISTOL, SND_SHOT_RIFLE, SND_SHOT_SHOTGUN, SND_SHOT_SMG,	//same order as Weapon::anim 3, 4, 5, 6
	SND_HIT,											//bullet hits a zombie
	SND_ZOMBIE_DIE,
	SND_HURT,											//player is hit
	SND_PICKUP, SND_HEAL, SND_SWAP,
	SND_EXIT,
	SND_UI_CLICK,
	SND_HEARTBEAT,										//low hp
	SND_COUNT
};

//Simple sound player on SDL3's own audio (no extra library).
//All WAV files must be 16-bit, mono, 44100 Hz (the ones I generated are).
//Up to VOICES sounds can play at the same time; each voice is an SDL audio stream bound to one device and SDL mixes them.
class AudioSystem {
public:
	static const int VOICES = 16;

private:
	SDL_AudioDeviceID device;
	SDL_AudioStream* voices[VOICES];
	Uint8* data[SND_COUNT];
	Uint32 length[SND_COUNT];
	Uint64 lastPlayed[SND_COUNT];
	Uint64 endTick[VOICES];				//when the sound on this voice stops (ms, SDL_GetTicks)
	int voiceSound[VOICES];				//which sound the voice plays
	Uint64 lastVocal;					//last time any zombie voice started
	SDL_AudioSpec spec;
	float master;
	bool ready;

	//anti-spam rules: the same sound can not restart before this many milliseconds
	static Uint32 defaultGapMs(int id) {
		switch (id) {
		case SND_HURT:        return 600;
		case SND_HIT:         return 70;
		case SND_ZOMBIE_DIE:  return 150;
		case SND_SWAP:        return 400;
		case SND_PICKUP:
		case SND_HEAL:        return 120;
		case SND_UI_CLICK:    return 80;
		default:              return 0;
		}
	}

	//how many copies of the same sound may play at the same time
	static int maxConcurrent(int id) {
		switch (id) {
		case SND_HIT:
		case SND_ZOMBIE_DIE:  return 2;
		case SND_SHOT_PISTOL:
		case SND_SHOT_RIFLE:
		case SND_SHOT_SHOTGUN:
		case SND_SHOT_SMG:    return 3;
		default:              return 1;
		}
	}

	//groans, growls and snarls share one "zombie voice": only one at a time, and not too soon after the last one
	static bool isVocal(int id) { return id >= SND_GROAN1 && id <= SND_SNARL; }
	static const Uint64 VOCAL_GAP_MS = 1500;

	int playingCount(int id, Uint64 now) const {
		int n = 0;
		for (int i = 0; i < VOICES; i++)
			if (voices[i] && endTick[i] > now && voiceSound[i] == id)
				n++;
		return n;
	}

	bool vocalPlaying(Uint64 now) const {
		for (int i = 0; i < VOICES; i++)
			if (voices[i] && endTick[i] > now && isVocal(voiceSound[i]))
				return true;
		return false;
	}

	static const char* fileName(int id) {
		switch (id) {
		case SND_GROAN1:       return "groan1.wav";
		case SND_GROAN2:       return "groan2.wav";
		case SND_GROAN3:       return "groan3.wav";
		case SND_GROWL1:       return "growl1.wav";
		case SND_GROWL2:       return "growl2.wav";
		case SND_SNARL:        return "snarl.wav";
		case SND_SHOT_PISTOL:  return "shot_pistol.wav";
		case SND_SHOT_RIFLE:   return "shot_rifle.wav";
		case SND_SHOT_SHOTGUN: return "shot_shotgun.wav";
		case SND_SHOT_SMG:     return "shot_smg.wav";
		case SND_HIT:          return "hit.wav";
		case SND_ZOMBIE_DIE:   return "zombie_die.wav";
		case SND_HURT:         return "hurt.wav";
		case SND_PICKUP:       return "pickup.wav";
		case SND_HEAL:         return "heal.wav";
		case SND_SWAP:         return "weapon_swap.wav";
		case SND_EXIT:         return "exit.wav";
		case SND_UI_CLICK:     return "ui_click.wav";
		default:               return "heartbeat.wav";
		}
	}

public:
	AudioSystem() : device(0), lastVocal(0), master(1.0f), ready(false) {
		for (int i = 0; i < VOICES; i++) {
			voices[i] = nullptr;
			endTick[i] = 0;
			voiceSound[i] = -1;
		}
		for (int i = 0; i < SND_COUNT; i++) {
			data[i] = nullptr;
			length[i] = 0;
			lastPlayed[i] = 0;
		}
		spec = SDL_AudioSpec{ SDL_AUDIO_S16, 1, 44100 };
	}

	//call after SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO). Returns false when there is no audio device:
	//the game keeps running, play() just does nothing.
	bool init(const std::string& folder = "Assets/Sounds/") {
		device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
		if (!device)
			return false;

		for (int i = 0; i < VOICES; i++) {
			voices[i] = SDL_CreateAudioStream(&spec, nullptr);
			if (voices[i])
				SDL_BindAudioStream(device, voices[i]);
		}

		const char* base = SDL_GetBasePath();
		for (int i = 0; i < SND_COUNT; i++) {
			std::string path = std::string(base ? base : "") + folder + fileName(i);
			SDL_AudioSpec s;
			if (!SDL_LoadWAV(path.c_str(), &s, &data[i], &length[i])) {
				data[i] = nullptr;
				continue;
			}
			//wrong format -> skip it instead of playing noise
			if (s.format != spec.format || s.channels != spec.channels || s.freq != spec.freq) {
				SDL_free(data[i]);
				data[i] = nullptr;
			}
		}
		ready = true;
		return true;
	}

	void unload() {
		for (int i = 0; i < VOICES; i++)
			if (voices[i]) {
				SDL_DestroyAudioStream(voices[i]);
				voices[i] = nullptr;
			}
		if (device) {
			SDL_CloseAudioDevice(device);
			device = 0;
		}
		for (int i = 0; i < SND_COUNT; i++)
			if (data[i]) {
				SDL_free(data[i]);
				data[i] = nullptr;
			}
		ready = false;
	}

	void setMasterVolume(float v) { master = (v < 0) ? 0 : (v > 1 ? 1 : v); }
	float getMasterVolume() const { return master; }

	//volume 0..1, pitch 1.0 = normal (0.9..1.1 gives variation), minGapMs = extra "do not restart this sound sooner than"
	//rule. play() also has built-in anti-spam rules (see defaultGapMs / maxConcurrent / isVocal above), so calling it
	//every frame from a collision is safe: repeated calls are simply ignored.
	void play(int id, float volume = 1.0f, float pitch = 1.0f, Uint32 minGapMs = 0) {
		if (!ready || id < 0 || id >= SND_COUNT || !data[id])
			return;
		Uint64 now = SDL_GetTicks();

		Uint32 gap = (minGapMs > defaultGapMs(id)) ? minGapMs : defaultGapMs(id);
		if (gap && lastPlayed[id] && now - lastPlayed[id] < gap)
			return;
		if (isVocal(id)) {
			if ((lastVocal && now - lastVocal < VOCAL_GAP_MS) || vocalPlaying(now))
				return;
		}
		else if (playingCount(id, now) >= maxConcurrent(id))
			return;

		for (int i = 0; i < VOICES; i++) {
			SDL_AudioStream* v = voices[i];
			if (v && endTick[i] <= now) {						//free voice
				SDL_SetAudioStreamGain(v, volume * master);
				SDL_SetAudioStreamFrequencyRatio(v, pitch);
				SDL_PutAudioStreamData(v, data[id], (int)length[id]);
				Uint64 ms = (Uint64)length[id] * 1000 / ((Uint64)spec.freq * 2);		//16-bit mono
				endTick[i] = now + (Uint64)((float)ms / pitch) + 30;
				voiceSound[i] = id;
				lastPlayed[id] = now;
				if (isVocal(id))
					lastVocal = now;
				return;
			}
		}
		//all voices busy: the sound is dropped
	}
};
