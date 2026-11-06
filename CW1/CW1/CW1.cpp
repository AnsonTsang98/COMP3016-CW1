//#include "SDL3/SDL_main.h"
#define NDEBUG 
#include "assert.h"
#include "GameObject.h"

#include "MainMenu.h"
#include "UpgradeData.h"
#include "UpgradeMenu.h"
#include "AudioSystem.h"

#include <iostream>
#include <array>
#include <string>

using namespace std;

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDK_CHARACTERS = 1;
const int MAP_ROWS = 30;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;
const int FOG_ROWS = MAP_ROWS * TILE_SIZE;
const int FOG_COLS = MAP_COLS * TILE_SIZE;
AudioSystem audio;

enum class FogState {
    Unexplored,
    Explored,
    Visible
};

enum class RunState {
    playing, escaped, dead
};

struct SDLState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    int width, height, logW, logH;
    const bool* keys;

    SDLState() : window(nullptr), renderer(nullptr), width(0), height(0), logW(0), logH(0), keys(SDL_GetKeyboardState(nullptr)) {}
};

struct Resources {
    //player anim
    const int ANIM_PLAYER_PISTOL = 0;
    const int ANIM_PLAYER_RIFLE = 1;
    const int ANIM_PLAYER_SHOTGUN = 2;
    const int ANIM_PLAYER_SMG = 3;
    const int ANIM_PLAYER_DEAD = 4;
    vector<Animation> playerAnims;

    //bullet anim
    const int ANIM_BULLET_MOVING = 0;
    const int ANIM_BULLET_HIT = 1;
    vector<Animation> bulletAnims;

    //enemy anim
    const int ANIM_ENEMY_WALK = 0;
    const int ANIM_ENEMY_CHASE = 1;
    const int ANIM_ENEMY_FWALK = 2;
    const int ANIM_ENEMY_FCHASE = 3;
    const int ANIM_ENEMY_DIE = 4;
    vector<Animation> enemyAnims;

    //enemy anim
    const int ANIM_SPEEDUP = 0;
    const int ANIM_MEDKIT = 1;
    const int ANIM_FLASHLIGHT = 2;
    const int ANIM_PISTOL = 3;
    const int ANIM_RIFLE = 4;
    const int ANIM_SHOTGUN = 5;
    const int ANIM_SMG = 6;
    vector<Animation> itemAnims;

    //screen anim
    Animation screenAnim = Animation(24, 4.0f);

    vector<SDL_Texture*> textures;
    SDL_Texture* texPPistol,* texPRifle,* texPShotgun,* texPSMG,* texPDead,* texGround,* texWall,* texBullet,* texBulletHit,
        * texNormalWalk,* texNormalChase,* texNormalDie,* texRageWalk,* texRageChase,* texRageDie,
        * texSpeedUp,* texMedKit,* texFlashlight,* texPistol,* texRifle,* texShotgun,* texSMG,* texBleed,
        * texMenuBg, * texMenuTitle, * texMenuButtons, * texUpTitle, * texUpRows, * texUpIcons, * texUpPips, * texUpPlus, * texExit;

    SDL_Texture* loadTexture(SDL_Renderer* renderer, const string& texPath) {
        //create surface from path
        string imgPath = (string)SDL_GetBasePath() + texPath;
        SDL_Surface* surface = SDL_LoadBMP(imgPath.c_str());    

        //remove white color as remove background
        Uint32 colorkey = SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format),nullptr, 255, 255, 255);
        SDL_SetSurfaceColorKey(surface, true, colorkey);

        //create texture
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST); //scale mode for a picture
        textures.push_back(tex);
        return tex;
    }

    Resources():texPPistol(nullptr), texPRifle(nullptr), texPShotgun(nullptr), texPSMG(nullptr), texPDead(nullptr), texGround(nullptr), texWall(nullptr), texBullet(nullptr),
        texBulletHit(nullptr), texNormalWalk(nullptr), texNormalChase(nullptr), texNormalDie(nullptr), texRageWalk(nullptr), texRageChase(nullptr), texRageDie(nullptr),
        texSpeedUp(nullptr), texMedKit(nullptr), texFlashlight(nullptr),  texPistol(nullptr),  texRifle(nullptr),  texShotgun(nullptr),  texSMG(nullptr), texBleed(nullptr),
        texMenuBg(nullptr), texMenuTitle(nullptr), texMenuButtons(nullptr), texUpTitle(nullptr), texUpRows(nullptr), texUpIcons(nullptr), texUpPips(nullptr), texUpPlus(nullptr),
        texExit(nullptr) {}

    void load(SDLState& state) {
        //player anims
        playerAnims.resize(5);
        playerAnims[ANIM_PLAYER_PISTOL] =
        playerAnims[ANIM_PLAYER_RIFLE] =
        playerAnims[ANIM_PLAYER_SHOTGUN] =
        playerAnims[ANIM_PLAYER_SMG] =
        playerAnims[ANIM_PLAYER_DEAD] = Animation(8, 1.0f);

        //bullet anims
        bulletAnims.resize(2);
        bulletAnims[ANIM_BULLET_MOVING] = Animation(3, 0.05f);
        bulletAnims[ANIM_BULLET_HIT] = Animation(4, 0.15f);

        //enemy anims
        enemyAnims.resize(5);
        enemyAnims[ANIM_ENEMY_WALK] = Animation(8, 2.0f);
        enemyAnims[ANIM_ENEMY_CHASE] = Animation(8, 1.0f);
        enemyAnims[ANIM_ENEMY_FWALK] = Animation(8, 2.0f);
        enemyAnims[ANIM_ENEMY_FCHASE] = Animation(8, 0.6f);
        enemyAnims[ANIM_ENEMY_DIE] = Animation(8, 1.6f);

        //item anims
        itemAnims.resize(7);
        itemAnims[ANIM_SPEEDUP] = Animation(8, 1.6f, SDL_rand(8));
        itemAnims[ANIM_MEDKIT] = Animation(8, 1.6f, SDL_rand(8));
        itemAnims[ANIM_FLASHLIGHT] = Animation(8, 1.6f, SDL_rand(8));
        itemAnims[ANIM_PISTOL] = Animation(8, 1.6f, SDL_rand(8));
        itemAnims[ANIM_RIFLE] = Animation(8, 1.6f, SDL_rand(8));
        itemAnims[ANIM_SHOTGUN] = Animation(8, 1.6f, SDL_rand(8));
        itemAnims[ANIM_SMG] = Animation(8, 1.6f, SDL_rand(8));

        //load texture from bmp file by path
        texPPistol = loadTexture(state.renderer, "Assets/main_pistol_walk.bmp");
        texPRifle = loadTexture(state.renderer, "Assets/main_rifle_walk.bmp");
        texPShotgun = loadTexture(state.renderer, "Assets/main_shotgun_walk.bmp");
        texPSMG = loadTexture(state.renderer, "Assets/main_smg_walk.bmp");
        texPDead = loadTexture(state.renderer, "Assets/main_dead_anim.bmp");
        texGround = loadTexture(state.renderer, "Assets/ground3.bmp");
        texWall = loadTexture(state.renderer, "Assets/wall2.bmp");
        texBullet = loadTexture(state.renderer, "Assets/bullet.bmp");
        texBulletHit = loadTexture(state.renderer, "Assets/bulletHit.bmp");
        texNormalWalk = loadTexture(state.renderer, "Assets/zombie_idle_walk.bmp");
        texNormalChase = loadTexture(state.renderer, "Assets/zombie_chase_walk.bmp");
        texNormalDie = loadTexture(state.renderer, "Assets/zombie_dead_anim.bmp");
        texRageWalk = loadTexture(state.renderer, "Assets/rage_idle_walk.bmp");
        texRageChase = loadTexture(state.renderer, "Assets/rage_chase_walk.bmp");
        texRageDie = loadTexture(state.renderer, "Assets/rage_dead_anim.bmp");
        texSpeedUp = loadTexture(state.renderer, "Assets/speedUp.bmp");
        texMedKit = loadTexture(state.renderer, "Assets/medkit_anim.bmp");
        texFlashlight = loadTexture(state.renderer, "Assets/flashlight_anim.bmp");
        texPistol = loadTexture(state.renderer, "Assets/pistol_anim.bmp");
        texRifle = loadTexture(state.renderer, "Assets/assault_rifle_anim.bmp");
        texShotgun = loadTexture(state.renderer, "Assets/shotgun_anim.bmp");
        texSMG = loadTexture(state.renderer, "Assets/smg_anim.bmp");
        texBleed = loadTexture(state.renderer, "Assets/bleed_anim_sheet.bmp");
        SDL_SetTextureBlendMode(texBleed, SDL_BLENDMODE_BLEND);
        texMenuBg = loadTexture(state.renderer, "Assets/menu_bg.bmp");
        texMenuTitle = loadTexture(state.renderer, "Assets/menu_title.bmp");
        texMenuButtons = loadTexture(state.renderer, "Assets/menu_buttons.bmp");
        texUpTitle = loadTexture(state.renderer, "Assets/upgrade_title.bmp");
        texUpRows = loadTexture(state.renderer, "Assets/upgrade_rows.bmp");
        texUpIcons = loadTexture(state.renderer, "Assets/upgrade_icons.bmp");
        texUpPips = loadTexture(state.renderer, "Assets/upgrade_pips.bmp");
        texUpPlus = loadTexture(state.renderer, "Assets/upgrade_plus.bmp");
        texExit = loadTexture(state.renderer, "Assets/exit_anim.bmp");
    }

    void unload() {
        for (SDL_Texture* tex : textures)
            SDL_DestroyTexture(tex);
    }
};

struct GameState {
    // Setup Fog Grids
    vector<vector<FogState>> fogGrid;
    vector<vector<bool>> wallGrid;
    // Setup Camera
    SDL_FRect mapViewport;
    vector<Weapon*> guns;
    vector<Zombie*> zombies;
    array<vector<GameObject>,2> layers;
    vector<GameObject> background, foreground, bullets;
    int playerIndex;
    float playerAngle;
    bool debugmode,pre,use;
    UpgradeData* upgrades = nullptr;
    RunState runState = RunState::playing;
    float endTimer = 0;         //time since the run ended (banner time before going back to the menu)
    bool nearExit = false;      //player is standing on the exit this frame
    vector<int> visibleCells;           //cells that are Visible this frame (used in step 2)
    vector<SDL_FPoint> visiblePoints;   //the same cells as screen points for the fog mask (step 2)
    vector<Uint8> fogPixels;            //pixel buffer for the fog overlay

    

    GameState(const SDLState& state) :playerIndex(-1), playerAngle(0), fogGrid(FOG_ROWS, vector<FogState>(FOG_COLS, FogState::Unexplored)),
        wallGrid(FOG_ROWS, vector<bool>(FOG_COLS, 0)), mapViewport(SDL_FRect{0,0,static_cast<float>(state.logW),static_cast<float>(state.logH)}),
    debugmode(false),pre(false),use(false){}
    GameObject& player() { return layers[LAYER_IDK_CHARACTERS][playerIndex]; }
    PlayerData& playerdata() { return layers[LAYER_IDK_CHARACTERS][playerIndex].data.player; }

    void unload() {
        for (Weapon* gun : guns)
            delete gun;
        for (Zombie* zombie : zombies)
            delete zombie;
    }
};

struct FogSystem {
    int width, height;
    SDL_Texture* entityBuffer,* fogMask;

    int fogW, fogH;
    SDL_Texture* fogOverlay;


    FogSystem(const SDLState& state) :entityBuffer(nullptr), fogMask(nullptr), fogOverlay(nullptr), fogW(0), fogH(0) {
        width = state.logW;
        height = state.logH;

        //custom blend mode for only take buffer color and the alpha collided between mask and buffer
        SDL_BlendMode fogBlendMode = SDL_ComposeCustomBlendMode(
            SDL_BLENDFACTOR_ZERO,              // Mask Color Factor
            SDL_BLENDFACTOR_ONE,               // Buffer Color Factor
            SDL_BLENDOPERATION_ADD,            // Color Operation
            SDL_BLENDFACTOR_ONE,               // Mask Alpha Factor
            SDL_BLENDFACTOR_ONE,               // Buffer Alpha Factor
            SDL_BLENDOPERATION_MINIMUM         // Alpha Operation
        );

        //create a texture to hold raw entity renderings
        entityBuffer = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET, width, height);

        //create a texture to act as the visibility mask
        fogMask = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET, width, height);

        //setup blend mode
        SDL_SetTextureBlendMode(fogMask, fogBlendMode);
        SDL_SetTextureBlendMode(entityBuffer, SDL_BLENDMODE_BLEND);

        //fog overlay: only the part of the map that is on screen (+2 so a fractional camera position is covered)
        fogW = state.logW + 2;
        fogH = state.logH + 2;
        fogOverlay = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, fogW, fogH);
        SDL_SetTextureBlendMode(fogOverlay, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(fogOverlay, SDL_SCALEMODE_NEAREST);

    }

    void unload() {
        SDL_DestroyTexture(entityBuffer);
        SDL_DestroyTexture(fogMask);
        SDL_DestroyTexture(fogOverlay);
    }
};

bool initSDL(SDLState&);
void cleanSDL(SDLState&);
void drawObject(const SDLState&, GameState&, GameObject&, float, float, float);
void update(const SDLState&, GameState&, Resources&, GameObject&, float);
void createTiles(SDLState&, GameState&, const Resources&);
void CastVisionRay(int, int, int, int, GameState&, const SDLState&);
void fogHandle(GameState&, const SDLState&, FogSystem);
void collisionResponse(const SDLState&, GameState&, Resources&, const SDL_FRect&, const SDL_FRect&, const SDL_FRect&, GameObject&, GameObject&, float);
void checkCollision(const SDLState&, GameState&, Resources&, GameObject&, GameObject&, float);
void restartGame(SDLState&, GameState&, Resources&, UpgradeData&);
void drawText(const SDLState&, const char*, float, float, float, Uint8, Uint8, Uint8);
void updateZombieSounds(GameState&, float);
void drawFog(const SDLState&, GameState&, FogSystem&);

//absolute values, so it is safe to call every time START is pressed (also when resuming a paused game)
void applyUpgrades(GameState& gs, const UpgradeData& up) {
    PlayerData& pd = gs.playerdata();

    float oldMax = pd.playerMaxHP;
    pd.playerMaxHP = up.maxHp();                       // base 100 + 20 per level
    pd.playerHP += pd.playerMaxHP - oldMax;            // new max HP also gives that HP now
    if (pd.playerHP > pd.playerMaxHP)
        pd.playerHP = pd.playerMaxHP;

    pd.playerSpeed = 100.0f * up.speedMult();          // 100 = your base speed
    gs.player().maxSpeed = pd.playerSpeed;
    pd.playerVision = 4 + up.visionBonus();            // 4 = your base vision (int)

    pd.weaponTimer.setLength((pd.gun->cooldown + pd.extraCooldown - up.cooldownMult()));
}

//throw the finished run away and build a fresh one (map, enemies, items, fog, player).
    //Only the UpgradeData is kept, because it lives outside the GameState.
void restartGame(SDLState& state, GameState& gs, Resources& res, UpgradeData& upgrades) {
    gs.unload();                    //delete the guns and zombies of the old run
    gs = GameState(state);          //brand new GameState
    gs.upgrades = &upgrades;
    createTiles(state, gs, res);
}

//debug font text, scaled and centred on cx (logical coordinates)
void drawText(const SDLState& state, const char* text, float cx, float y, float scale, Uint8 r, Uint8 g, Uint8 b) {
    float w = (float)SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale;
    SDL_SetRenderScale(state.renderer, scale, scale);
    SDL_SetRenderDrawColor(state.renderer, r, g, b, 255);
    SDL_RenderDebugText(state.renderer, (cx - w / 2) / scale, y / scale, text);
    SDL_SetRenderScale(state.renderer, 1.0f, 1.0f);
}

void updateZombieSounds(GameState& gs, float deltaTime) {
    static float timer = 0.0f;
    timer -= deltaTime;
    if (timer > 0.0f || gs.playerdata().state == PlayerState::dead)
        return;

    const float RANGE = 260.0f;                         //pixels (about 8 tiles)
    const Vec2 p = gs.player().position;
    GameObject* nearest = nullptr;
    float best = RANGE;
    for (GameObject& o : gs.layers[LAYER_IDK_CHARACTERS]) {
        if (o.type != ObjectType::enemy || o.data.enemy.state == EnemyState::dead)
            continue;
        float dx = o.position.x - p.x;
        float dy = o.position.y - p.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < best) {
            best = d;
            nearest = &o;
        }
    }
    if (!nearest) {
        timer = 0.25f;                                  //nobody near: check again soon
        return;
    }

    float close = 1.0f - best / RANGE;                  //0 = far .. 1 = touching
    int id;
    if (nearest->data.enemy.state == EnemyState::attack)
        id = SND_SNARL;
    else if (nearest->data.enemy.z->score != 1)         //rage zombie
        id = SND_GROWL1 + SDL_rand(2);
    else
        id = SND_GROAN1 + SDL_rand(3);

    float volume = 0.2f + 0.8f * close * close;
    //quieter when the zombie is not in your vision (behind a wall / outside the light)
    int zx = (int)(nearest->position.x + TILE_SIZE / 2);
    int zy = (int)(nearest->position.y + TILE_SIZE / 2);
    bool seen = zx >= 0 && zx < FOG_COLS && zy >= 0 && zy < FOG_ROWS && gs.fogGrid[zy][zx] == FogState::Visible;
    if (!seen)
        volume *= 0.6f;

    audio.play(id, volume, 0.9f + 0.2f * SDL_randf());
    timer = 2.5f + 4.0f * (1.0f - close) + 2.0f * SDL_randf();   //closer = more often
}

//draw the fog of the visible part of the map with one texture
void drawFog(const SDLState& state, GameState& gs, FogSystem& fs) {
    const int vx = (int)floorf(gs.mapViewport.x);
    const int vy = (int)floorf(gs.mapViewport.y);
    const int w = fs.fogW;
    const int h = fs.fogH;

    gs.fogPixels.resize((size_t)w * h * 4);
    Uint8* px = gs.fogPixels.data();
    for (int y = 0; y < h; y++) {
        const int my = vy + y;
        for (int x = 0; x < w; x++) {
            const int mx = vx + x;
            Uint8 a = 0;                                    //outside the map or visible = no fog
            if (mx >= 0 && mx < FOG_COLS && my >= 0 && my < FOG_ROWS) {
                FogState f = gs.fogGrid[my][mx];
                a = (f == FogState::Unexplored) ? 255 : (f == FogState::Explored) ? 165 : 0;
            }
            px[0] = 0;
            px[1] = 0;
            px[2] = 0;
            px[3] = a;
            px += 4;
        }
    }
    SDL_UpdateTexture(fs.fogOverlay, nullptr, gs.fogPixels.data(), w * 4);

    SDL_FRect dst{ vx - gs.mapViewport.x, vy - gs.mapViewport.y, (float)w, (float)h };
    SDL_RenderTexture(state.renderer, fs.fogOverlay, nullptr, &dst);
}




int main() {
    //initialize SDL
    SDLState state;
    state.width = 1280;
    state.height = 720;
    state.logW = 640;
    state.logH = 320;
    if (!initSDL(state)) { return 1; }

    //load game assets
    Resources res;
    res.load(state);

    //init audio
    audio.init();                       // false = no audio device, the game still runs
    audio.setMasterVolume(0.7f);        // 0..1, change to taste


    //setup game
    GameState gs(state);
    createTiles(state, gs, res);
    uint64_t prevTime = SDL_GetTicks();
    FogSystem fs(state);

    //init menu
    MainMenu menu;
    menu.init(res.texMenuBg, res.texMenuTitle, res.texMenuButtons,
        res.texPRifle, res.texNormalChase, res.texRageChase, state.logW, state.logH);

    //init upgrade
    UpgradeData upgrades;
    upgrades.load();                  // "save.json" next to the exe; first launch = defaults, no error
    gs.upgrades = &upgrades;          // use your GameState variable name

    UpgradeMenu upMenu;
    upMenu.init(res.texMenuBg, res.texUpTitle, res.texUpRows, res.texUpIcons, res.texUpPips,
        res.texUpPlus, res.texMenuButtons, state.logW, state.logH);
    bool inUpgrade = false;

    
    bool running = true;
    bool gaming = false;
    while (running) {
        //main menu
        SDL_Event event{ 0 };
        if (!gaming) {
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
                case SDL_EVENT_WINDOW_RESIZED: {
                    state.width = event.window.data1;
                    state.height = event.window.data2;
                    break;
                }
                }

                if (inUpgrade) {
                    switch (upMenu.handleEvent(event, state.renderer)) {
                    case UpgradeAction::bought: {
                        audio.play(SND_PICKUP, 0.8f);
                        upgrades.save();                    // saved on every purchase
                        break;
                    }
                    case UpgradeAction::back: {
                        audio.play(SND_UI_CLICK);
                        inUpgrade = false;
                        upgrades.save();
                        menu.reset();
                        break;
                    }
                    default:
                        break;
                    }
                }
                else {
                    switch (menu.handleEvent(event, state.renderer)) {
                    case MenuAction::start: {
                        audio.play(SND_UI_CLICK);
                        applyUpgrades(gs, upgrades);        // function from step 6
                        gaming = true;
                        prevTime = SDL_GetTicks();
                        break;
                    }
                    case MenuAction::upgrade: {
                        audio.play(SND_UI_CLICK);
                        inUpgrade = true;
                        upMenu.open(&upgrades);
                        break;
                    }
                    case MenuAction::quit: {
                        running = false;
                        break;
                    }
                    default:
                        break;
                    }
                }
            }

            SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
            SDL_RenderClear(state.renderer);
            if (inUpgrade) {
                upMenu.update();
                upMenu.draw(state.renderer);
            }
            else {
                menu.update();
                menu.draw(state.renderer);
            }
            SDL_RenderPresent(state.renderer);
        }
        //gameloop
        while (gaming) {
            uint64_t nowTime = SDL_GetTicks();
            float deltaTime = (nowTime - prevTime) / 1000.0f;
            
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                    gaming = false;
                    break;
                }
                case SDL_EVENT_WINDOW_RESIZED: {
                    state.width = event.window.data1;
                    state.height = event.window.data2;
                    break;
                }
                }
            }

            //the run only ends when the player escapes through the exit (E) or dies
            if (gs.runState == RunState::playing && gs.playerdata().state == PlayerState::dead)
                gs.runState = RunState::dead;
            if (gs.runState != RunState::playing) {
                gs.endTimer += deltaTime;
                if (gs.endTimer >= ((gs.runState == RunState::dead) ? 3.0f : 1.5f)) {
                    upgrades.save();                        //keep the score earned in this run
                    restartGame(state, gs, res, upgrades);  //everything else restarts
                    gaming = false;
                    menu.reset();
                    prevTime = SDL_GetTicks();
                    continue;                               //gaming is false, so this leaves the game loop
                }
            }

            //update all objects (frozen after escaping, but the death animation keeps playing)
            if (gs.runState != RunState::escaped) {
                for (auto& layer : gs.layers)
                    for (GameObject& obj : layer) {
                        update(state, gs, res, obj, deltaTime);
                    }

                for (GameObject& bullet : gs.bullets) {
                    update(state, gs, res, bullet, deltaTime);
                }
            }

            //sounds
            if (gs.runState == RunState::playing) {
                updateZombieSounds(gs, deltaTime);

                static float heartTimer = 0.0f;
                heartTimer -= deltaTime;
                if (heartTimer <= 0.0f && gs.playerdata().playerHP < gs.playerdata().playerMaxHP * 0.3f) {
                    audio.play(SND_HEARTBEAT, 0.6f);
                    heartTimer = 0.9f;
                }
            }

            //calculatte viewport
            gs.mapViewport.x = (gs.player().position.x + TILE_SIZE / 2) - gs.mapViewport.w / 2;
            gs.mapViewport.y = (gs.player().position.y + TILE_SIZE / 2) - gs.mapViewport.h / 2;

            fogHandle(gs, state, fs);

            //update bleeding screen effect
            float screenBleedRatio = (1 - ((gs.playerdata().playerHP < 0) ? 0 : gs.playerdata().playerHP) / gs.playerdata().playerMaxHP);
            res.screenAnim.step(deltaTime);
            SDL_FRect screenDst{ 0 ,0 ,(float)state.logW,(float)state.logH };
            SDL_FRect screenSrc{ state.logW * (res.screenAnim.currentFrame() % 4),state.logH * (int)(res.screenAnim.currentFrame() / 4),state.logW,state.logH };
            SDL_SetTextureAlphaMod(res.texBleed, 200 * screenBleedRatio);

            //draw entity to buffer texture
            SDL_SetRenderTarget(state.renderer, fs.entityBuffer);
            SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 0);
            SDL_RenderClear(state.renderer);

            for (GameObject& obj : gs.layers[LAYER_IDK_CHARACTERS]) {
                if (obj.type == ObjectType::enemy) {
                    drawObject(state, gs, obj, TILE_SIZE, TILE_SIZE, deltaTime);
                }
            }

            //blend the buffer and fog mask
            SDL_SetRenderTarget(state.renderer, fs.entityBuffer);
            SDL_RenderTexture(state.renderer, fs.fogMask, nullptr, nullptr);

            //prepare render
            SDL_SetRenderTarget(state.renderer, nullptr);
            SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
            SDL_RenderClear(state.renderer);

            //draw background
            for (GameObject& obj : gs.background) {
                SDL_FRect dst{
                    obj.position.x - gs.mapViewport.x,
                    obj.position.y - gs.mapViewport.y,
                    static_cast<float>(obj.texture->w),
                    static_cast<float>(obj.texture->h)
                };
                //off screen: skip
                if (dst.x + dst.w < 0 || dst.x > state.logW || dst.y + dst.h < 0 || dst.y > state.logH)
                    continue;

                SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
            }

            //draw foreground
            /*for (GameObject& obj : gs.foreground) {
                SDL_FRect dst{
                    obj.position.x - gs.mapViewport.x,
                    obj.position.y - gs.mapViewport.y,
                    static_cast<float>(obj.texture->w),
                    static_cast<float>(obj.texture->h)
                };
                SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
            }*/

            //draw all objects
            for (auto& layer : gs.layers)
                for (GameObject& obj : layer) {
                    if (obj.type != ObjectType::enemy && !(obj.type == ObjectType::item && !obj.data.item.active))
                        drawObject(state, gs, obj, TILE_SIZE, TILE_SIZE, deltaTime);
                }

            //draw entity from buffer to screen
            SDL_RenderTexture(state.renderer, fs.entityBuffer, nullptr, nullptr);

            //draw bullets
            for (GameObject& bullet : gs.bullets) {
                if (bullet.data.bullet.state != BulletState::inactive)
                    drawObject(state, gs, bullet, bullet.collider.w, bullet.collider.h, deltaTime);
            }

            //fog
            drawFog(state, gs, fs);

            SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);

            /*for (int y = 0; y < FOG_ROWS; y++)
                for (int x = 0; x < FOG_COLS; x++) {
                    SDL_FRect fogRect = { x - gs.mapViewport.x, y - gs.mapViewport.y, 1.0f, 1.0f };

                    if (gs.fogGrid[y][x] == FogState::Unexplored) {
                        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255); // Shroud
                        SDL_RenderFillRect(state.renderer, &fogRect);
                    }
                    else if (gs.fogGrid[y][x] == FogState::Explored) {
                        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 165); // Fog
                        SDL_RenderFillRect(state.renderer, &fogRect);
                    }
                }*/



            //draw bleeding screen overlay represent player hp
            SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 128 * screenBleedRatio); // Blood
            SDL_RenderFillRect(state.renderer, &screenDst);
            SDL_RenderTexture(state.renderer, res.texBleed, &screenSrc, &screenDst);


            //exit hint and end-of-run banner
            if (gs.nearExit && gs.runState == RunState::playing)
                drawText(state, "PRESS E TO ESCAPE", state.logW / 2.0f, state.logH - 40.0f, 1.0f, 90, 230, 120);
            if (gs.runState != RunState::playing) {
                SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 120);
                SDL_RenderFillRect(state.renderer, &screenDst);
                if (gs.runState == RunState::dead)
                    drawText(state, "YOU DIED", state.logW / 2.0f, state.logH / 2.0f - 16.0f, 4.0f, 210, 40, 40);
                else
                    drawText(state, "ESCAPED!", state.logW / 2.0f, state.logH / 2.0f - 16.0f, 4.0f, 90, 230, 120);
                char scoreText[48];
                SDL_snprintf(scoreText, sizeof(scoreText), "SCORE: %d", upgrades.score);
                drawText(state, scoreText, state.logW / 2.0f, state.logH / 2.0f + 24.0f, 1.0f, 210, 210, 216);
            }



            //debug use info
            if (gs.debugmode) {
                string msg = "S: " + to_string(static_cast<int>(gs.playerdata().state)) +
                    ", B: " + to_string(gs.bullets.size()) + ", gun_CD: " + to_string(gs.playerdata().gun->cooldown) +
                    ", E: " + to_string(gs.use) + ", hp: " + to_string(gs.playerdata().playerHP) + ", Frame: " + to_string(res.screenAnim.currentFrame()) +
                    ", W: " + to_string(res.screenAnim.currentFrame() % 4) + ", H: " + to_string((int)(res.screenAnim.currentFrame() / 6));
                SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
                SDL_RenderDebugText(state.renderer, 5, 5, msg.c_str());
            }

            //render
            SDL_RenderPresent(state.renderer);

            prevTime = nowTime;
        }
    }
    upgrades.save();
    //cout<< SDL_GetBasePath() <<endl; // show base path
    gs.unload();
    res.unload();
    fs.unload();
    audio.unload(); 
    cleanSDL(state);
    return 0;
}

//use for initialize SDL
bool initSDL(SDLState& state) {
    bool initSuccess = true;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) { initSuccess = false; }

    //create window
    state.window = SDL_CreateWindow("Escape Anything", state.width, state.height, SDL_WINDOW_RESIZABLE);
    if (!state.window) { cleanSDL(state); initSuccess = false; }

    //create renderer
    state.renderer = SDL_CreateRenderer(state.window, nullptr);
    if (!state.renderer) { cleanSDL(state); initSuccess = false; }
    SDL_SetRenderVSync(state.renderer,1);

    //configure presentation
    SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    return initSuccess;
}

//use for close SDL
void cleanSDL(SDLState& state) {
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}

//use for draw Game Object
void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float width, float height, float deltaTime) {
    float srcX = (obj.currentAnimation != -1) ? obj.animations[obj.currentAnimation].currentFrame() * width : (obj.spriteFrame - 1) * width;
    SDL_FRect src{
        srcX,
        0,
        width,
        height
    };

    SDL_FRect dst{
        obj.position.x - gs.mapViewport.x,
        obj.position.y - gs.mapViewport.y,
        width,
        height
    };

    //off screen: skip (an object that is flashing still finishes its flash timer below)
    if (!obj.shouldFlash && (dst.x + width < 0 || dst.x > state.logW || dst.y + height < 0 || dst.y > state.logH))
        return;


    if (obj.shouldFlash)
        SDL_SetTextureColorModFloat(obj.texture, 5.0f, 1.0f, 1.0f);
    if (obj.type == ObjectType::level || obj.type == ObjectType::exit)
        SDL_RenderTexture(state.renderer, obj.texture, &src, &dst);
    else if(obj.type == ObjectType::player)
        SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, gs.playerAngle, nullptr, SDL_FLIP_NONE);
    else
        SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, atan2(obj.direction.y, obj.direction.x) * 180 / 3.14159265f, nullptr, SDL_FLIP_NONE);
    SDL_SetTextureColorModFloat(obj.texture, 1.0f, 1.0f, 1.0f);

    if (obj.flashTimer.step(deltaTime)) {
        obj.shouldFlash = false;
    }

    //debug mode
    if (gs.debugmode) {
        SDL_FRect rectA{
        obj.position.x + obj.collider.x - gs.mapViewport.x,
        obj.position.y + obj.collider.y - gs.mapViewport.y,
        obj.collider.w,
        obj.collider.h
        };
        SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 150);
        SDL_RenderFillRect(state.renderer, &rectA);
        SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);
    }
}

//use for update Game Object
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime) {
    //update animation
    if (obj.currentAnimation != -1)
        obj.animations[obj.currentAnimation].step(deltaTime);

    //update player if Game Object is player
    if (obj.type == ObjectType::player) {
        //handle player mouse inputs
        float rawMouseX, rawMouseY, mouseX, mouseY;
        SDL_MouseButtonFlags mouseB = SDL_GetMouseState(&rawMouseX, &rawMouseY);        
        SDL_RenderCoordinatesFromWindow(state.renderer, rawMouseX, rawMouseY, &mouseX, &mouseY);

        //handle player vision
        float cx = TILE_SIZE / 2 + obj.position.x - gs.mapViewport.x;
        float cy = TILE_SIZE / 2 + obj.position.y - gs.mapViewport.y;
        float dx = mouseX - cx;
        float dy = mouseY - cy;

        //calculatte viewport with mouse offset
        //gs.mapViewport.x = (mouseX - (state.logW / 2)) / 8  + (gs.player().position.x + TILE_SIZE / 2) - gs.mapViewport.w / 2;
        //gs.mapViewport.y = (mouseY - (state.logH / 2)) / 6  + (gs.player().position.y + TILE_SIZE / 2) - gs.mapViewport.h / 2;

        //update player angle dynamically if the mouse has moved away from the player center
        if ((abs(dx) > 1.0f || abs(dy) > 1.0f) && gs.playerdata().state != PlayerState::dead) {
            float mouseAngleRad = atan2(dy, dx);
            gs.playerAngle = mouseAngleRad * 180 / 3.14159265f;
        }

        
        //handle player movement
        Vec2 direction;
        gs.use = false;
        gs.nearExit = false;
        if (gs.playerdata().state != PlayerState::dead) {
            if (state.keys[SDL_SCANCODE_A])
                direction.x -= 1;
            if (state.keys[SDL_SCANCODE_D])
                direction.x += 1;
            if (state.keys[SDL_SCANCODE_W])
                direction.y -= 1;
            if (state.keys[SDL_SCANCODE_S])
                direction.y += 1;
            if (state.keys[SDL_SCANCODE_E])
                gs.use = true;
        }

        //handle horizontal speed slowdown
        if (direction.x)
            obj.direction.x = direction.x;
        else if (obj.velocity.x) {
            const float factor = (obj.velocity.x > 0) ? -1.5f : 1.5f;
            float amount = factor * obj.acceleration * deltaTime;
            if (abs(obj.velocity.x) < abs(amount)) {
                obj.velocity.x = 0;
            }
            else {
                obj.velocity.x += amount;
            }
        }

        //handle vertical speed slowdown
        if (direction.y)
            obj.direction.y = direction.y;
        else if (obj.velocity.y) {
            const float factor = (obj.velocity.y > 0) ? -1.5f : 1.5f;
            float amount = factor * obj.acceleration * deltaTime;
            if (abs(obj.velocity.y) < abs(amount)) {
                obj.velocity.y = 0;
            }
            else {
                obj.velocity.y += amount;
            }
        }

        //lambda function
        auto const spawnBullet = [&gs,&res,&obj,&cx,&cy,&dx,&dy]() {
            //spawn bullet
            GameObject bullet;
            bullet.data.bullet = BulletData();
            bullet.type = ObjectType::bullet;
            bullet.texture = res.texBullet;
            bullet.currentAnimation = res.ANIM_BULLET_MOVING;
            bullet.animations = res.bulletAnims;

            //caculate the bullet direction
            float d = sqrtf(dx * dx + dy * dy);
            float variationX, variationY = variationX = gs.playerdata().gun->spread - gs.upgrades->accuracy();
            variationX = SDL_rand(variationX) - variationX / 2;
            variationY = SDL_rand(variationY) - variationY / 2;
            bullet.direction = Vec2((dx + variationX) / d, (dy + variationY) / d);

            bullet.collider = SDL_FRect{ 0,0,static_cast<float>(res.texBullet->h),static_cast<float>(res.texBullet->h) };
            bullet.position = Vec2(cx + gs.mapViewport.x + (12 * bullet.direction.x), cy + gs.mapViewport.y + (12 * bullet.direction.y));
            bullet.velocity = Vec2(obj.velocity.x + 200 * bullet.direction.x, obj.velocity.y + 200 * bullet.direction.y);

            //overwrit inactive bullet slot
            bool foundInactive = false;
            for (int i = 0; i < gs.bullets.size() && !foundInactive; i++)
                if (gs.bullets[i].data.bullet.state == BulletState::inactive) {
                    foundInactive = true;
                    gs.bullets[i] = bullet;
                }
            //push a new when no inactive bullet slot
            if (!foundInactive)
                gs.bullets.push_back(bullet);
        };

        //handle shooting
        Timer& weaponTimer = gs.playerdata().weaponTimer;
        weaponTimer.step(deltaTime);
        if (mouseB & SDL_BUTTON_LMASK) {
            if (gs.playerdata().state != PlayerState::dead && weaponTimer.isTimeout()) {
                weaponTimer.reset();
                int bullet = gs.playerdata().gun->bullet;
                audio.play(SND_SHOT_PISTOL + (gs.playerdata().gun->anim - 3), 0.7f, 0.95f + 0.1f * SDL_randf());
                for (int n = 0; n < bullet; n++)
                    spawnBullet();
            }
        }

        //debug mode
        if (!state.keys[SDL_SCANCODE_F10]&&gs.pre) {
            gs.debugmode = !gs.debugmode;
        }
        gs.pre = state.keys[SDL_SCANCODE_F10];

        //handle player FSM
        switch (gs.playerdata().state) {
            case PlayerState::idle: {
                if (direction.x != 0 || direction.y != 0) {
                    gs.playerdata().state = PlayerState::walking;
                }
                if (obj.currentAnimation != -1 && obj.animations[obj.currentAnimation].isDone()) {
                    gs.playerdata().lastAnim = obj.currentAnimation;
                    obj.currentAnimation = -1;
                    obj.spriteFrame = 1;
                }
                break;
            }
            case PlayerState::walking: {
                if (direction.x == 0 && direction.y == 0) {
                    gs.playerdata().state = PlayerState::idle;
                }
                if (obj.currentAnimation == -1) {
                    obj.currentAnimation = gs.playerdata().lastAnim;
                }
                break;
            }
            case PlayerState::dead: {
                if (obj.currentAnimation != -1 && obj.animations[obj.currentAnimation].isDone()) {
                    obj.currentAnimation = -1;
                    obj.spriteFrame = 8;
                }
            }
        }

        //update player velocity
        obj.velocity += direction * obj.acceleration * deltaTime;
        if (abs(obj.velocity.x) > obj.maxSpeed)
            obj.velocity.x = direction.x * obj.maxSpeed;
        if (abs(obj.velocity.y) > obj.maxSpeed)
            obj.velocity.y = direction.y * obj.maxSpeed;
    }
    //update bullet if Game Object is bullet
    else if (obj.type == ObjectType::bullet) {
        //handle bullet FSM
        switch (obj.data.bullet.state) {
            case BulletState::moving: {
                if (obj.position.x - gs.mapViewport.x < 0 || obj.position.x - gs.mapViewport.x > state.logW ||
                    obj.position.y - gs.mapViewport.y < 0 || obj.position.y - gs.mapViewport.y > state.logH)
                    obj.data.bullet.state = BulletState::inactive;
                break;
            }
            case BulletState::colliding: {
                obj.data.bullet.state = BulletState::inactive;
                break;
            }
        }
        
    }
    //update enemy if Game Object is enemy
    else if (obj.type == ObjectType::enemy) {
        EnemyData& enemydata = obj.data.enemy;
        //lambda function
        auto const zombieChangeState = [&](EnemyState state) {
            enemydata.state = state;
            switch (state) {
                case EnemyState::idle: {
                    enemydata.stateTimer.setLength(enemydata.idleTime);
                    if (enemydata.z->score == 1) {
                        obj.texture = res.texNormalWalk;
                    }
                    else {
                        obj.texture = res.texRageWalk;
                    }
                    obj.currentAnimation = -1;
                    obj.spriteFrame = 1;
                    obj.velocity = Vec2(0, 0);
                    break;
                }
                case EnemyState::roaming: {
                    enemydata.stateTimer.setLength(enemydata.roamingTime);
                    if (enemydata.z->score == 1) {
                        obj.texture = res.texNormalWalk;
                        obj.currentAnimation = res.ANIM_ENEMY_WALK;
                    }
                    else {
                        obj.texture = res.texRageWalk;
                        obj.currentAnimation = res.ANIM_ENEMY_FWALK;
                    }
                    obj.velocity = enemydata.z->speed * obj.direction;
                    break;
                }
                case EnemyState::attack: {
                    enemydata.stateTimer.setLength(enemydata.attackTime);
                    if (enemydata.z->score == 1) {
                        obj.texture = res.texNormalChase;
                        obj.currentAnimation = res.ANIM_ENEMY_CHASE;
                    }
                    else {
                        obj.texture = res.texRageChase;
                        obj.currentAnimation = res.ANIM_ENEMY_FCHASE;
                    }
                    obj.velocity = enemydata.z->speed * obj.direction;
                    break;
                }
            }
        };

        //lambda function
        const auto playerInRange = [&]() {
            //check player nearby or not
            float dx = gs.player().position.x - obj.position.x;
            float dy = gs.player().position.y - obj.position.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (enemydata.state != EnemyState::dead && d < 100) {
                obj.direction = Vec2(dx / d, dy / d);
                zombieChangeState(EnemyState::attack);
                return true && gs.playerdata().state != PlayerState::dead;
            }
            return false;
        };

        //handle enemy FSM
        switch (enemydata.state) {
            case EnemyState::idle: {
                if (playerInRange()) {}
                else if (enemydata.stateTimer.step(deltaTime)) {
                    int angle = SDL_rand(360);
                    obj.direction = Vec2(cos(angle), sin(angle));
                    zombieChangeState(EnemyState::roaming);
                }
                break;
            }
            case EnemyState::roaming: {
                if (playerInRange()) {}
                else if (enemydata.stateTimer.step(deltaTime)) {
                    zombieChangeState(EnemyState::idle);
                }
                break;
            }
            case EnemyState::attack: {
                enemydata.cooldownTimer.step(deltaTime);
                if (enemydata.cooldownTimer.isTimeout() && gs.playerdata().state != PlayerState::dead) {
                    obj.velocity = enemydata.z->speed * obj.direction;
                    if (playerInRange())
                        enemydata.stateTimer.reset();
                    else if (enemydata.stateTimer.step(deltaTime)) {
                        zombieChangeState(EnemyState::idle);
                    }
                }
                else if(gs.playerdata().state == PlayerState::dead)
                    obj.velocity = Vec2(0, 0);
                break;
            }
            case EnemyState::damaged: {
                if (enemydata.damagedTimer.step(deltaTime)) {
                    if (!playerInRange()) {
                        zombieChangeState(EnemyState::idle);
                    }
                }
                break;
            }
            case EnemyState::dead: {
                if (obj.currentAnimation != -1 && obj.animations[obj.currentAnimation].isDone()) {
                    obj.currentAnimation = -1;
                    obj.spriteFrame = 8;
                }
                break;
            }
        }
    }
    else if(obj.type == ObjectType::item && obj.data.item.type == ItemType::weapon)
        obj.data.item.gun->pickUpCooldown.step(deltaTime);
    //update Game Object location
    obj.position += obj.velocity * deltaTime;

    //check the Game Object collide with other Game Object or not
    /*for (auto& layer : gs.layers) {
        for (GameObject& objB : layer) {
            if (&obj != &objB) {
                checkCollision(state,gs,res,obj,objB,deltaTime);
            }
        }
    }*/
    //only moving objects react to collisions (as "objA", walls / items / the exit never do anything)
    if (obj.type == ObjectType::player || obj.type == ObjectType::enemy || obj.type == ObjectType::bullet) {
        for (auto& layer : gs.layers) {
            for (GameObject& objB : layer) {
                if (&obj == &objB)
                    continue;
                //further than 2 tiles away it can not touch us
                if (fabsf(objB.position.x - obj.position.x) > TILE_SIZE * 2 ||
                    fabsf(objB.position.y - obj.position.y) > TILE_SIZE * 2)
                    continue;
                checkCollision(state, gs, res, obj, objB, deltaTime);
            }
        }
    }

}

//use for handle the collision
void collisionResponse(const SDLState& state, GameState& gs, Resources& res, const SDL_FRect& rectA, const SDL_FRect& rectB, const SDL_FRect& rectC,
    GameObject& objA, GameObject& objB, float deltaTime) {
    //lambda function
    const auto genericResponse = [&]() {
        if (rectC.w < rectC.h) {
            //horizontal collision
            objA.position.x += (objA.position.x > objB.position.x) ? rectC.w : -rectC.w;
            objA.velocity.x = 0;
        }
        else {
            //vertical collision
            objA.position.y += (objA.position.y > objB.position.y) ? rectC.h : -rectC.h;
            objA.velocity.y = 0;
        }
    };

    //player situation
    if (objA.type == ObjectType::player) {
        switch (objB.type) {
            case ObjectType::level: {
                genericResponse();
                break;
            }
            case ObjectType::enemy: {
                if (objB.data.enemy.state != EnemyState::dead && gs.playerdata().state != PlayerState::dead) {
                    float knockback = 100.0f;
                    float dx = objA.position.x - objB.position.x;
                    float dy = objA.position.y - objB.position.y;
                    float d = sqrtf(dx * dx + dy * dy);
                    float dirX = dx / d;
                    float dirY = dy / d;
                    if (rectC.w < rectC.h) {
                        //horizontal collision
                        objA.position.x += (rectC.w + 10) * (objA.position.x > objB.position.x) ? 1 : -1;
                        objA.velocity.x = dirX * knockback;
                    }
                    else {
                        //vertical collision
                        objA.position.y += (rectC.h + 10) * (objA.position.y > objB.position.y) ? 1 : -1;
                        objA.velocity.y = dirY * knockback;
                    }
                    objA.shouldFlash = true;
                    objA.flashTimer.reset();
                    objA.data.player.playerHP -= objB.data.enemy.z->damage;
                    audio.play(SND_HURT, 0.9f, 0.95f + 0.1f * SDL_randf(), 300);
                    if (gs.playerdata().playerHP <= 0) {
                        gs.playerdata().state = PlayerState::dead;
                        objA.texture = res.texPDead;
                        objA.currentAnimation = res.ANIM_PLAYER_DEAD;
                        objA.velocity = Vec2(0, 0);
                    }
                }
                break;
            }
            case ObjectType::exit: {
                gs.nearExit = true;
                if (gs.use && gs.runState == RunState::playing) {
                    gs.runState = RunState::escaped;    //main loop shows the banner, saves and returns to the menu
                    audio.play(SND_EXIT, 0.9f);
                    gs.endTimer = 0;
                    objA.velocity = Vec2(0, 0);
                }
                break;
            }
            case ObjectType::item: {
                PlayerData& pd = objA.data.player;
                ItemData& itemdata = objB.data.item;
                if(gs.use && itemdata.active && !(itemdata.type == ItemType::weapon && !itemdata.gun->pickUpCooldown.isTimeout())){
                    audio.play((itemdata.type == ItemType::heal) ? SND_HEAL : (itemdata.type == ItemType::weapon) ? SND_SWAP : SND_PICKUP, 0.8f);
                    if (itemdata.type != ItemType::weapon) {
                        itemdata.active = false;
                        objB.collider = { 0,0,0,0 };
                    }
                    else {
                        itemdata.gun->pickUpCooldown.reset();
                        pd.gun->pickUpCooldown.reset();
                    }
                    switch (itemdata.type) {
                        case ItemType::speedup: {
                            objA.maxSpeed += itemdata.value;
                            break;
                        }
                        case ItemType::heal: {
                            pd.playerHP += itemdata.value;
                            if (pd.playerHP > pd.playerMaxHP)
                                pd.playerHP = pd.playerMaxHP;
                            break;
                        }
                        case ItemType::flashlight: {
                            pd.playerFov += itemdata.value * 10;
                            pd.playerVision += itemdata.value;
                            if (pd.playerFov > 180)
                                pd.playerFov = 180;
                            break;
                        }
                        case ItemType::weapon: {
                            switch (pd.gun->anim) {
                                case 3: {
                                    objB.texture = res.texPistol;
                                    break;
                                }
                                case 4: {
                                    objB.texture = res.texRifle;
                                    break;
                                }
                                case 5: {
                                    objB.texture = res.texShotgun;
                                    break;
                                }
                                case 6: {
                                    objB.texture = res.texSMG;
                                    break;
                                }
                            }
                            switch (itemdata.gun->anim) {
                                case 3: {
                                    objA.texture = res.texPPistol;
                                    objA.currentAnimation = res.ANIM_PLAYER_PISTOL;
                                    break;
                                }
                                case 4: {
                                    objA.texture = res.texPRifle;
                                    objA.currentAnimation = res.ANIM_PLAYER_RIFLE;
                                    break;
                                }
                                case 5: {
                                    objA.texture = res.texPShotgun;
                                    objA.currentAnimation = res.ANIM_PLAYER_SHOTGUN;
                                    break;
                                }
                                case 6: {
                                    objA.texture = res.texPSMG;
                                    objA.currentAnimation = res.ANIM_PLAYER_SMG;
                                    break;
                                }
                            }
                            objB.currentAnimation = pd.gun->anim;
                            pd.weaponTimer.setLength(pd.extraCooldown + itemdata.gun->cooldown - gs.upgrades->cooldownMult());
                            swap(pd.gun,itemdata.gun);
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    //bullet situation
    else if (objA.type == ObjectType::bullet) {
        bool passthrough = false;
        switch (objA.data.bullet.state) {
            case BulletState::moving: {
                switch (objB.type) {
                    case ObjectType::level: {
                        break;
                    }
                    case ObjectType::enemy: {
                        EnemyData& enemydata = objB.data.enemy;
                        if (enemydata.state != EnemyState::dead) {
                            objB.direction = Vec2(-objA.direction.x, -objA.direction.y);
                            objB.position = Vec2(objB.position.x + (objA.direction.x * 2), objB.position.y + (objA.direction.y * 2));
                            objB.velocity = objB.direction * enemydata.z->speed;
                            objB.texture = (objB.data.enemy.z->score == 1) ? res.texNormalWalk : res.texRageWalk;
                            objB.currentAnimation = (objB.data.enemy.z->score == 1) ? res.ANIM_ENEMY_WALK : res.ANIM_ENEMY_FWALK;
                            objB.shouldFlash = true;
                            objB.flashTimer.reset();
                            enemydata.state = EnemyState::damaged;
                            enemydata.z->hp -= gs.playerdata().gun->damage;
                            audio.play(SND_HIT, 0.6f, 0.9f + 0.2f * SDL_randf(), 40);
                            if (enemydata.z->hp <= 0 && enemydata.state != EnemyState::dead) {
                                enemydata.state = EnemyState::dead;
                                objB.texture = (objB.data.enemy.z->score == 1)? res.texNormalDie:res.texRageDie;
                                objB.currentAnimation = res.ANIM_ENEMY_DIE;
                                objB.velocity = Vec2(0, 0);
                                objB.collider = { 0,0,0,0 };
                                gs.upgrades->score += enemydata.z->score;
                                audio.play(SND_ZOMBIE_DIE, 0.9f);
                            }
                        }
                        else {
                            passthrough = true;
                        }
                        break;
                    }
                    case ObjectType::item:
                    case ObjectType::exit: {
                        passthrough = true;
                        break;
                    }
                }
                if (!passthrough) {
                    genericResponse();
                    objA.velocity *= 0;
                    objA.data.bullet.state = BulletState::colliding;
                    objA.texture = res.texBulletHit;
                    objA.currentAnimation = res.ANIM_BULLET_HIT;
                }
                break;
            }
        }
    }
    //enemy situation
    else if (objA.type == ObjectType::enemy) {
        if (objA.data.enemy.state == EnemyState::dead)
            return; 
        if (objB.type == ObjectType::player) {
            objA.data.enemy.cooldownTimer.reset();
            objA.velocity = Vec2(0, 0);
        }
        else if (objB.type == ObjectType::item || objB.type == ObjectType::exit ||
            (objB.type == ObjectType::enemy && objB.data.enemy.state == EnemyState::dead)) {
            //zombies walk over items, the exit and dead bodies
        }
        else
            genericResponse();
    }
}

//use for check the collision
void checkCollision(const SDLState& state, GameState& gs, Resources& res, GameObject& a, GameObject& b, float deltaTime) {
    SDL_FRect rectA{
        a.position.x + a.collider.x,
        a.position.y + a.collider.y,
        a.collider.w,
        a.collider.h
    };
    SDL_FRect rectB{
        b.position.x + b.collider.x,
        b.position.y + b.collider.x,
        b.collider.w,
        b.collider.h
    };
    SDL_FRect rectC{ 0 };

    if (SDL_GetRectIntersectionFloat(&rectA,&rectB,&rectC))
        //intersection
        collisionResponse(state,gs,res,rectA,rectB,rectC,a,b,deltaTime);
}

//use for create the map and Game Object onn the map
void createTiles(SDLState& state, GameState& gs, const Resources& res) {
    /*
        1 - Ground
        2 - Rage Enemy
        3 - Normal Enemy
        4 - Player
        5 - Wall
        6 - Items
        7 - Exit (stand on it and press E)
    */

    //the map
    short map[MAP_ROWS][MAP_COLS] = {
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 4, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 5, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 6, 0, 0, 0, 0, 0, 5, 0, 0, 6, 0, 0, 0, 2, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 6, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 6, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 5, 5, 0, 0, 0, 0, 0, 0, 5, 0, 0, 6, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 6, 0, 0, 0, 0, 5, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 5, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 5, 5, 0, 0, 5, 5, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 5, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 3, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 5,
        5, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 6, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 5, 0, 0, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 5, 5, 0, 0, 0, 0, 0, 3, 0, 0, 5, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 0, 0, 0, 5, 0, 0, 2, 0, 2, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 6, 0, 0, 0, 0, 6, 0, 5,
        5, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    };
    //background map
    short backgroundMap[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    //foreground map
    /*short foregroundMap[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };*/

    //lambda function
    const auto loadMap = [&state, &gs, &res](short layer[MAP_ROWS][MAP_COLS]) {
        //lambda function
        const auto createObject = [&state](int r, int c, SDL_Texture* tex, ObjectType type) {
            //create Game Object
            GameObject o;
            o.type = type;
            o.position = Vec2(c * TILE_SIZE, r * TILE_SIZE);
            o.texture = tex;
            o.collider = { 0,0,TILE_SIZE,TILE_SIZE };
            return o;
        };

        //loading map
        for (int r = 0; r < MAP_ROWS; r++)
            for (int c = 0; c < MAP_COLS; c++)
                switch (layer[r][c]) {
                    case 1: {//Ground
                        GameObject ground = createObject(r, c, res.texGround, ObjectType::level);
                        gs.background.push_back(ground);
                        break;
                    }
                    case 2: {//Rage Enemy
                        GameObject enemy = createObject(r, c, res.texRageWalk, ObjectType::enemy);
                        enemy.data.enemy = EnemyData();
                        EnemyData& ed = enemy.data.enemy;
                        ed.z = new Rage;
                        gs.zombies.push_back(ed.z);
                        enemy.currentAnimation = res.ANIM_ENEMY_FWALK;
                        enemy.animations = res.enemyAnims;
                        enemy.acceleration = 300;
                        enemy.maxSpeed = ed.z->speed;
                        enemy.collider = { 0,0,32,32 };
                        gs.layers[LAYER_IDK_CHARACTERS].push_back(enemy);
                        break;
                    }
                    case 3: {//Normal Enemy
                        GameObject enemy = createObject(r, c, res.texNormalWalk, ObjectType::enemy);
                        enemy.data.enemy = EnemyData();
                        EnemyData& ed = enemy.data.enemy;
                        ed.z = new Normal;
                        gs.zombies.push_back(ed.z);
                        enemy.currentAnimation = res.ANIM_ENEMY_WALK;
                        enemy.animations = res.enemyAnims;
                        enemy.acceleration = 300;
                        enemy.maxSpeed = ed.z->speed;
                        enemy.collider = { 0,0,32,32 };
                        gs.layers[LAYER_IDK_CHARACTERS].push_back(enemy);
                        break;
                    }
                    case 4: {//Player
                        GameObject player = createObject(r, c, res.texPPistol, ObjectType::player);
                        player.data.player = PlayerData();
                        PlayerData& pd = player.data.player;
                        pd.gun = new Pistol;
                        gs.guns.push_back(pd.gun);
                        pd.weaponTimer.setLength(pd.gun->cooldown + pd.extraCooldown);
                        player.animations = res.playerAnims;
                        player.acceleration = 300;
                        player.maxSpeed = pd.playerSpeed;
                        player.collider = { 10,10,12,12 };
                        player.currentAnimation = res.ANIM_PLAYER_PISTOL;
                        gs.layers[LAYER_IDK_CHARACTERS].push_back(player);
                        gs.playerIndex = gs.layers[LAYER_IDK_CHARACTERS].size() - 1;
                        break;
                    }
                    case 5: {//Wall
                        GameObject wall = createObject(r, c, res.texWall, ObjectType::level);
                        gs.layers[LAYER_IDX_LEVEL].push_back(wall);
                        for (int i = 0; i < TILE_SIZE; i++)
                            for (int k = 0; k < TILE_SIZE; k++)
                                gs.wallGrid[r * TILE_SIZE + i][c * TILE_SIZE + k] = 1;
                        break;
                    }
                    case 6: {//Items
                        GameObject item = createObject(r, c, res.texWall, ObjectType::item);
                        item.data.item = ItemData();
                        ItemData& itemdata = item.data.item;
                        int ca = -1;
                        switch (itemdata.type) {
                            case ItemType::speedup: {
                                ca = res.ANIM_SPEEDUP;
                                item.texture = res.texSpeedUp;
                                break;
                            }
                            case ItemType::heal: {
                                ca = res.ANIM_MEDKIT;
                                item.texture = res.texMedKit;
                                break;
                            }
                            case ItemType::flashlight: {
                                ca = res.ANIM_FLASHLIGHT;
                                item.texture = res.texFlashlight;
                                break;
                            }
                            case ItemType::weapon: {
                                switch ((int)abs(itemdata.value)) {
                                    case 1: {
                                        itemdata.gun = new Pistol;
                                        item.texture = res.texPistol;
                                        break;
                                    }
                                    case 2: {
                                        itemdata.gun = new Rifle;
                                        item.texture = res.texRifle;
                                        break;
                                    }
                                    case 3: {
                                        itemdata.gun = new Shotgun;
                                        item.texture = res.texShotgun;
                                        break;
                                    }
                                    case 4: {
                                        itemdata.gun = new Submachine;
                                        item.texture = res.texSMG;
                                        break;
                                    }
                                }
                                gs.guns.push_back(itemdata.gun);
                                ca = itemdata.gun->anim;
                                break;
                            }
                        }

                        
                        item.direction = Vec2(0,0);
                        item.currentAnimation = ca;
                        item.animations = res.itemAnims;
                        item.collider = { 0,0,32,32 };
                        gs.layers[LAYER_IDX_LEVEL].push_back(item);
                        break;
                    }
                    case 7: {//Exit
                        GameObject exitObj = createObject(r, c, res.texExit, ObjectType::exit);
                        exitObj.animations = { Animation(8, 1.6f) };
                        exitObj.currentAnimation = 0;
                        gs.layers[LAYER_IDX_LEVEL].push_back(exitObj);
                        break;
                    }

                }
    };
    loadMap(map);
    loadMap(backgroundMap);
    //loadMap(foregroundMap);
    //debug use
    assert(gs.playerIndex != -1);
}

//mark one fog cell as visible (once per frame) and remember it
inline void markVisible(GameState& gs, int x, int y) {
    if (gs.fogGrid[y][x] == FogState::Visible)
        return;
    gs.fogGrid[y][x] = FogState::Visible;
    gs.visibleCells.push_back(y * FOG_COLS + x);
    gs.visiblePoints.push_back(SDL_FPoint{ x - gs.mapViewport.x, y - gs.mapViewport.y });
}


//Bresenham's Line Trace for Line of Sight
void CastVisionRay(int x0, int y0, int x1, int y1, GameState& gs, const SDLState& state) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int cx = x0;
    int cy = y0;
    int wv = 8;

    while (true) {
        //stop when outside the fog map
        if (cx < 0 || cx >= FOG_COLS || cy < 0 || cy >= FOG_ROWS) break;
        /*gs.fogGrid[cy][cx] = FogState::Visible;
        SDL_RenderPoint(state.renderer, cx - gs.mapViewport.x, cy - gs.mapViewport.y);*/
        markVisible(gs, cx, cy);
        //block by wall
        if (wv <= 0) break;
        if (gs.wallGrid[cy][cx]) wv--;
        //stop when achive the end point
        if (cx == x1 && cy == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy && e2 < dx) {
            cx += sx;
            err -= dy;
        }
        else {
            if (e2 > -dy) {
                err -= dy;
                cx += sx;
            }
            if (e2 < dx) {
                err += dx;
                cy += sy;
            }   
        }
    }
}

//handle fog
void fogHandle(GameState& gs, const SDLState& state, FogSystem fs) {
    //prepare fog mask
    SDL_SetRenderTarget(state.renderer, fs.fogMask);
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 0);
    SDL_RenderClear(state.renderer);
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);

    const GameObject& obj = gs.player();
    const PlayerData& objdata = gs.playerdata();
    //reset fog state
    /*for (int y = 0; y < FOG_ROWS; y++)
        for (int x = 0; x < FOG_COLS; x++)
            if (gs.fogGrid[y][x] == FogState::Visible)
                gs.fogGrid[y][x] = FogState::Explored;*/
    for (int idx : gs.visibleCells)
        gs.fogGrid[idx / FOG_COLS][idx % FOG_COLS] = FogState::Explored;
    gs.visibleCells.clear();
    gs.visiblePoints.clear();

    //cast rays in limited sector
    int playerVision = objdata.playerVision * TILE_SIZE / 2;
    int steps = objdata.playerFov * 4;
    int cx = TILE_SIZE / 2 + obj.position.x;
    int cy = TILE_SIZE / 2 + obj.position.y;

    for (int i = 0; i < steps; i++) {
        float angle = objdata.playerFov * i / steps + gs.playerAngle - (objdata.playerFov / 2);
        float rad = angle * 3.14159265 / 180;

        int targetX = cos(rad) * playerVision + cx;
        int targetY = sin(rad) * playerVision + cy;
        CastVisionRay(cx, cy, targetX, targetY, gs, state);
    }

    //player centre always visible
    /*if (cy >= 0 && cy < FOG_ROWS && cx >= 0 && cx < FOG_COLS) {
        gs.fogGrid[cy][cx] = FogState::Visible;
        SDL_RenderPoint(state.renderer, cx - gs.mapViewport.x, cy - gs.mapViewport.y);
    }*/
    if (cy >= 0 && cy < FOG_ROWS && cx >= 0 && cx < FOG_COLS)
        markVisible(gs, cx, cy);

    //draw all visible cells into the fog mask with ONE call (the render target is still fogMask here)
    if (!gs.visiblePoints.empty())
        SDL_RenderPoints(state.renderer, gs.visiblePoints.data(), (int)gs.visiblePoints.size());

}