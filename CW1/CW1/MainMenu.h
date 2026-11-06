#pragma once
#include "SDL3/SDL.h"
#include "Animation.h"

#include <cmath>
#include <cstring>

//what the menu wants the main loop to do
enum class MenuAction {
	none, start, upgrade, quit
};

//Main menu: START / UPGRADE / QUIT buttons, mouse + keyboard, a little animated stand-off in the background.
//The menu does not own any texture: load them through Resources::loadTexture so Resources::unload() frees them.
class MainMenu {
public:
	static const int BUTTON_COUNT = 3;		//0 = START, 1 = UPGRADE, 2 = QUIT
	static constexpr float BTN_W = 128, BTN_H = 32;
	static constexpr float TITLE_W = 384, TITLE_H = 64;
	static constexpr float CHAR_SIZE = 96;	//32px sprites drawn at 3x (integer scale keeps the pixels sharp)

private:
	SDL_Texture* texBg, * texTitle, * texButtons, * texPlayer, * texZombie, * texRage;
	SDL_FRect buttonRect[BUTTON_COUNT];
	Animation playerAnim, zombieAnim, rageAnim;
	float logW, logH, time;
	int selected;							//highlighted button (keyboard or mouse hover)
	int pressed;							//button the left mouse button went down on, -1 = none
	Uint64 lastTicks;

	int buttonAt(float x, float y) const {
		SDL_FPoint p{ x, y };
		for (int i = 0; i < BUTTON_COUNT; i++)
			if (SDL_PointInRectFloat(&p, &buttonRect[i]))
				return i;
		return -1;
	}

	static MenuAction actionFor(int button) {
		switch (button) {
			case 0:  return MenuAction::start;
			case 1:  return MenuAction::upgrade;
			default: return MenuAction::quit;
		}
	}

	void drawCharacter(SDL_Renderer* r, SDL_Texture* tex, int frame, float x, float y, double angle) const {
		SDL_FRect src{ frame * 32.0f, 0, 32, 32 };
		SDL_FRect dst{ x, y, CHAR_SIZE, CHAR_SIZE };
		SDL_RenderTextureRotated(r, tex, &src, &dst, angle, nullptr, SDL_FLIP_NONE);
	}

public:
	MainMenu() : texBg(nullptr), texTitle(nullptr), texButtons(nullptr), texPlayer(nullptr), texZombie(nullptr), texRage(nullptr),
		playerAnim(8, 0.8f), zombieAnim(8, 0.8f, 3), rageAnim(8, 0.5f, 5),
		logW(640), logH(320), time(0), selected(0), pressed(-1), lastTicks(0) {
		for (int i = 0; i < BUTTON_COUNT; i++)
			buttonRect[i] = SDL_FRect{ 0, 0, BTN_W, BTN_H };
	}

	//player / zombie / rage = the walk sprite sheets that are already loaded for the game
	void init(SDL_Texture* bg, SDL_Texture* title, SDL_Texture* buttons,
		SDL_Texture* player, SDL_Texture* zombie, SDL_Texture* rage, int logicalW, int logicalH) {
		texBg = bg; texTitle = title; texButtons = buttons;
		texPlayer = player; texZombie = zombie; texRage = rage;
		logW = (float)logicalW;
		logH = (float)logicalH;
		for (int i = 0; i < BUTTON_COUNT; i++)
			buttonRect[i] = SDL_FRect{ (logW - BTN_W) / 2, 120.0f + i * 44.0f, BTN_W, BTN_H };
	}

	//call when the menu is shown again (for example after the game)
	void reset() {
		time = 0;
		selected = 0;
		pressed = -1;
		lastTicks = 0;
	}

	//feed every SDL event to this while the menu is open
	MenuAction handleEvent(SDL_Event& e, SDL_Renderer* renderer) {
		switch (e.type) {
			case SDL_EVENT_KEY_DOWN: {
				if (e.key.repeat)
					break;
				switch (e.key.key) {
					case SDLK_UP:
					case SDLK_W:
						selected = (selected + BUTTON_COUNT - 1) % BUTTON_COUNT;
						break;
					case SDLK_DOWN:
					case SDLK_S:
						selected = (selected + 1) % BUTTON_COUNT;
						break;
					case SDLK_RETURN:
					case SDLK_SPACE:
						return actionFor(selected);
					case SDLK_ESCAPE:
						return MenuAction::quit;
					default:
						break;
				}
				break;
			}
			case SDL_EVENT_MOUSE_MOTION: {
				//window pixels -> logical (640x320) coordinates, works with the letterbox
				SDL_ConvertEventToRenderCoordinates(renderer, &e);
				int i = buttonAt(e.motion.x, e.motion.y);
				if (i != -1)
					selected = i;
				break;
			}
			case SDL_EVENT_MOUSE_BUTTON_DOWN: {
				if (e.button.button != SDL_BUTTON_LEFT)
					break;
				SDL_ConvertEventToRenderCoordinates(renderer, &e);
				pressed = buttonAt(e.button.x, e.button.y);
				if (pressed != -1)
					selected = pressed;
				break;
			}
			case SDL_EVENT_MOUSE_BUTTON_UP: {
				//the click counts on release, so the same click can not fire a shot at the start of the game
				if (e.button.button != SDL_BUTTON_LEFT)
					break;
				SDL_ConvertEventToRenderCoordinates(renderer, &e);
				int released = buttonAt(e.button.x, e.button.y);
				int was = pressed;
				pressed = -1;
				if (was != -1 && was == released)
					return actionFor(was);
				break;
			}
			default:
				break;
		}
		return MenuAction::none;
	}

	//call once per menu frame (it measures its own delta time)
	void update() {
		Uint64 now = SDL_GetTicks();
		float dt = (lastTicks == 0) ? 0.0f : (now - lastTicks) / 1000.0f;
		if (dt > 0.1f)
			dt = 0.1f;
		lastTicks = now;

		time += dt;
		playerAnim.step(dt);
		zombieAnim.step(dt);
		rageAnim.step(dt);
	}

	void draw(SDL_Renderer* r) const {
		//background
		SDL_RenderTexture(r, texBg, nullptr, nullptr);

		//stand-off: the player on the left, zombies (rotated 180 degrees to face left) on the right
		drawCharacter(r, texPlayer, playerAnim.currentFrame(), 36, 200, 0);
		drawCharacter(r, texZombie, zombieAnim.currentFrame(), 440, 206, 180);
		drawCharacter(r, texRage, rageAnim.currentFrame(), 520, 190, 180);

		//title, bobbing by whole pixels
		float bob = std::round(1.5f * std::sin(time * 2.0f));
		SDL_FRect titleDst{ (logW - TITLE_W) / 2, 12.0f + bob, TITLE_W, TITLE_H };
		SDL_RenderTexture(r, texTitle, nullptr, &titleDst);

		//buttons: frame 0 = normal, frame 1 = highlighted, one row per button
		for (int i = 0; i < BUTTON_COUNT; i++) {
			SDL_FRect src{ (selected == i) ? BTN_W : 0.0f, i * BTN_H, BTN_W, BTN_H };
			SDL_FRect dst = buttonRect[i];
			if (pressed == i)
				dst.y += 1;						//small "pushed in" effect
			SDL_RenderTexture(r, texButtons, &src, &dst);
		}

		//hint text (SDL's built-in debug font is 8x8)
		const char* hint = "W/S or MOUSE: select    ENTER or CLICK: confirm";
		float hintW = (float)std::strlen(hint) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
		SDL_SetRenderDrawColor(r, 120, 120, 130, 255);
		SDL_RenderDebugText(r, (logW - hintW) / 2, logH - 14, hint);

		//fade in from black when the menu opens
		float fade = 1.0f - time / 0.6f;
		if (fade > 0.0f) {
			SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(r, 0, 0, 0, (Uint8)(255.0f * fade));
			SDL_RenderFillRect(r, nullptr);
			SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
		}
	}
};
