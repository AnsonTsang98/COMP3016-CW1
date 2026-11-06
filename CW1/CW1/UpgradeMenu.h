#pragma once
#include "SDL3/SDL.h"
#include "UpgradeData.h"

#include <cmath>
#include <cstdio>
#include <cstring>

//what the upgrade page wants the main loop to do
enum class UpgradeAction {
	none,
	bought,		//something was bought -> good moment to save the JSON
	back		//leave the page
};

//Upgrade page: four rows (MAX HP / SPEED / VISION / FIREARMS) + BACK. Mouse and keyboard.
//It does not own textures (load them through Resources::loadTexture) and does not own the data:
//call open(&yourUpgradeData) each time the page is shown.
class UpgradeMenu {
public:
	static const int ROW_COUNT = UP_COUNT;		//rows 0..3 = upgrades, ROW_COUNT = BACK
	static constexpr float ROW_X = 80, ROW_Y = 64, ROW_STEP = 44, ROW_W = 480, ROW_H = 40;
	static constexpr float PLUS_W = 32, PLUS_H = 24;
	static constexpr float BTN_W = 128, BTN_H = 32, BACK_Y = 266;
	static constexpr float TITLE_W = 256, TITLE_H = 48;

private:
	SDL_Texture* texBg, * texTitle, * texRows, * texIcons, * texPips, * texPlus, * texButtons;
	UpgradeData* data;
	SDL_FRect rowRect[ROW_COUNT + 1];			//last one = BACK
	float logW, logH, time, msgTimer;
	int selected, pressed;
	bool msgGood;
	char msg[48];
	Uint64 lastTicks;

	int itemAt(float x, float y) const {
		SDL_FPoint p{ x, y };
		for (int i = 0; i <= ROW_COUNT; i++)
			if (SDL_PointInRectFloat(&p, &rowRect[i]))
				return i;
		return -1;
	}

	void say(const char* text, bool good) {
		std::snprintf(msg, sizeof(msg), "%s", text);
		msgGood = good;
		msgTimer = 1.4f;
	}

	UpgradeAction activate(int item) {
		if (item == ROW_COUNT)
			return UpgradeAction::back;
		if (!data)
			return UpgradeAction::none;
		if (data->buy(item)) {
			say("UPGRADED!", true);
			return UpgradeAction::bought;
		}
		say(data->isMax(item) ? "ALREADY AT MAX LEVEL" : "NOT ENOUGH SCORE", false);
		return UpgradeAction::none;
	}

	static int percent(float multiplier) {
		return (int)std::lround((multiplier - 1.0f) * 100.0f);
	}

	//"now > next level" text for one row
	void effectText(int type, char* out, size_t size) const {
		int lv = data->level[type];
		bool max = data->isMax(type);
		int nx = max ? lv : lv + 1;
		switch (type) {
			case UP_MAXHP:
				std::snprintf(out, size, max ? "%d (MAX)" : "%d > %d", (int)UpgradeData::maxHpAt(lv), (int)UpgradeData::maxHpAt(nx));
				break;
			case UP_SPEED:
				std::snprintf(out, size, max ? "x%.1f (MAX)" : "x%.1f > x%.1f", UpgradeData::speedMultAt(lv), UpgradeData::speedMultAt(nx));
				break;
			case UP_VISION:
				std::snprintf(out, size, max ? "RADIUS +%d (MAX)" : "RADIUS +%d > +%d", UpgradeData::visionBonusAt(lv), UpgradeData::visionBonusAt(nx));
				break;
			default:
				std::snprintf(out, size, max ? "accuracy +%d% (MAX)" : "accuracy +%d% > +%d%",
					UpgradeData::accuracyAt(lv), UpgradeData::accuracyAt(nx));
				break;
		}
	}

	void text(SDL_Renderer* r, float x, float y, Uint8 R, Uint8 G, Uint8 B, const char* s) const {
		SDL_SetRenderDrawColor(r, R, G, B, 255);
		SDL_RenderDebugText(r, x, y, s);
	}

public:
	UpgradeMenu() : texBg(nullptr), texTitle(nullptr), texRows(nullptr), texIcons(nullptr), texPips(nullptr),
		texPlus(nullptr), texButtons(nullptr), data(nullptr),
		logW(640), logH(320), time(0), msgTimer(0), selected(0), pressed(-1), msgGood(true), lastTicks(0) {
		msg[0] = '\0';
		for (int i = 0; i <= ROW_COUNT; i++)
			rowRect[i] = SDL_FRect{ 0, 0, 0, 0 };
	}

	//menuButtons = the same menu_buttons.bmp the main menu uses (row 3 = BACK)
	void init(SDL_Texture* bg, SDL_Texture* title, SDL_Texture* rows, SDL_Texture* icons, SDL_Texture* pips,
		SDL_Texture* plus, SDL_Texture* menuButtons, int logicalW, int logicalH) {
		texBg = bg; texTitle = title; texRows = rows; texIcons = icons; texPips = pips; texPlus = plus; texButtons = menuButtons;
		logW = (float)logicalW;
		logH = (float)logicalH;
		for (int i = 0; i < ROW_COUNT; i++)
			rowRect[i] = SDL_FRect{ ROW_X, ROW_Y + i * ROW_STEP, ROW_W, ROW_H };
		rowRect[ROW_COUNT] = SDL_FRect{ (logW - BTN_W) / 2, BACK_Y, BTN_W, BTN_H };
	}

	//call every time the page is shown
	void open(UpgradeData* upgrades) {
		data = upgrades;
		time = 0;
		msgTimer = 0;
		selected = 0;
		pressed = -1;
		lastTicks = 0;
	}

	UpgradeAction handleEvent(SDL_Event& e, SDL_Renderer* renderer) {
		switch (e.type) {
			case SDL_EVENT_KEY_DOWN: {
				if (e.key.repeat)
					break;
				switch (e.key.key) {
					case SDLK_UP:
					case SDLK_W:
						selected = (selected + ROW_COUNT) % (ROW_COUNT + 1);
						break;
					case SDLK_DOWN:
					case SDLK_S:
						selected = (selected + 1) % (ROW_COUNT + 1);
						break;
					case SDLK_RETURN:
					case SDLK_SPACE:
						return activate(selected);
					case SDLK_ESCAPE:
						return UpgradeAction::back;
					default:
						break;
				}
				break;
			}
			case SDL_EVENT_MOUSE_MOTION: {
				SDL_ConvertEventToRenderCoordinates(renderer, &e);
				int i = itemAt(e.motion.x, e.motion.y);
				if (i != -1)
					selected = i;
				break;
			}
			case SDL_EVENT_MOUSE_BUTTON_DOWN: {
				if (e.button.button != SDL_BUTTON_LEFT)
					break;
				SDL_ConvertEventToRenderCoordinates(renderer, &e);
				pressed = itemAt(e.button.x, e.button.y);
				if (pressed != -1)
					selected = pressed;
				break;
			}
			case SDL_EVENT_MOUSE_BUTTON_UP: {
				if (e.button.button != SDL_BUTTON_LEFT)
					break;
				SDL_ConvertEventToRenderCoordinates(renderer, &e);
				int released = itemAt(e.button.x, e.button.y);
				int was = pressed;
				pressed = -1;
				if (was != -1 && was == released)
					return activate(was);
				break;
			}
			default:
				break;
		}
		return UpgradeAction::none;
	}

	void update() {
		Uint64 now = SDL_GetTicks();
		float dt = (lastTicks == 0) ? 0.0f : (now - lastTicks) / 1000.0f;
		if (dt > 0.1f)
			dt = 0.1f;
		lastTicks = now;
		time += dt;
		if (msgTimer > 0.0f)
			msgTimer -= dt;
	}

	void draw(SDL_Renderer* r) const {
		SDL_RenderTexture(r, texBg, nullptr, nullptr);

		//darken the background so the rows are easy to read
		SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(r, 0, 0, 0, 150);
		SDL_RenderFillRect(r, nullptr);
		SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

		SDL_FRect titleDst{ (logW - TITLE_W) / 2, 6.0f, TITLE_W, TITLE_H };
		SDL_RenderTexture(r, texTitle, nullptr, &titleDst);

		char buf[64];
		if (data) {
			std::snprintf(buf, sizeof(buf), "SCORE: %d", data->score);
			text(r, (logW - std::strlen(buf) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2, 52, 210, 210, 216, buf);
		}

		for (int i = 0; i < ROW_COUNT; i++) {
			const SDL_FRect& rc = rowRect[i];
			float y = rc.y + ((pressed == i) ? 1.0f : 0.0f);

			SDL_FRect rowSrc{ 0.0f, ((selected == i ? 4 : 0) + i) * ROW_H, ROW_W, ROW_H };
			SDL_FRect rowDst{ rc.x, y, ROW_W, ROW_H };
			SDL_RenderTexture(r, texRows, &rowSrc, &rowDst);

			SDL_FRect iconSrc{ i * 32.0f, 0, 32, 32 };
			SDL_FRect iconDst{ rc.x + 4, y + 4, 32, 32 };
			SDL_RenderTexture(r, texIcons, &iconSrc, &iconDst);

			if (!data)
				continue;

			effectText(i, buf, sizeof(buf));
			text(r, rc.x + 44, y + 25, 150, 150, 160, buf);

			//level pips (frame 0 = empty, 1 = filled, 8x8 each, drawn 8px apart + 2px gap)
			for (int k = 0; k < UpgradeData::maxLevel(i); k++) {
				SDL_FRect pSrc{ (k < data->level[i]) ? 8.0f : 0.0f, 0, 8, 8 };
				SDL_FRect pDst{ rc.x + 300 + k * 10.0f, y + 16, 8, 8 };
				SDL_RenderTexture(r, texPips, &pSrc, &pDst);
			}

			//+ button: frame 0 = can buy, 1 = cannot, 2 = MAX
			int frame = data->isMax(i) ? 2 : (data->canBuy(i) ? 0 : 1);
			SDL_FRect bSrc{ frame * PLUS_W, 0, PLUS_W, PLUS_H };
			SDL_FRect bDst{ rc.x + ROW_W - 36, y + 8, PLUS_W, PLUS_H };
			SDL_RenderTexture(r, texPlus, &bSrc, &bDst);

			if (frame != 2) {
				std::snprintf(buf, sizeof(buf), "COST %d", data->cost(i));
				float w = std::strlen(buf) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
				if (frame == 0)
					text(r, bDst.x - 4 - w, y + 16, 210, 210, 216, buf);
				else
					text(r, bDst.x - 4 - w, y + 16, 120, 120, 130, buf);
			}
		}

		//message line under the rows
		if (msgTimer > 0.0f && msg[0] != '\0') {
			float w = std::strlen(msg) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
			if (msgGood)
				text(r, (logW - w) / 2, 246, 90, 200, 90, msg);
			else
				text(r, (logW - w) / 2, 246, 210, 60, 60, msg);
		}

		//BACK = row 3 of menu_buttons.bmp (frame 1 = highlighted)
		const SDL_FRect& bk = rowRect[ROW_COUNT];
		SDL_FRect bkSrc{ (selected == ROW_COUNT) ? BTN_W : 0.0f, 3 * BTN_H, BTN_W, BTN_H };
		SDL_FRect bkDst = bk;
		if (pressed == ROW_COUNT)
			bkDst.y += 1;
		SDL_RenderTexture(r, texButtons, &bkSrc, &bkDst);

		const char* hint = "W/S or MOUSE: select   ENTER or CLICK: buy   ESC: back";
		float hintW = (float)std::strlen(hint) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
		text(r, (logW - hintW) / 2, logH - 14, 120, 120, 130, hint);

		float fade = 1.0f - time / 0.4f;
		if (fade > 0.0f) {
			SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(r, 0, 0, 0, (Uint8)(255.0f * fade));
			SDL_RenderFillRect(r, nullptr);
			SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
		}
	}
};
