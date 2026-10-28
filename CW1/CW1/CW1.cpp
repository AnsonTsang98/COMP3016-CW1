//#include "SDL3/SDL_main.h"
#include "GameObject.h"

#include <iostream>
#include <array>
#include <string>

using namespace std;

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDK_CHARACTERS = 1;
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;

struct SDLState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    int width, height, logW, logH;
    const bool* keys;

    SDLState() : window(nullptr), renderer(nullptr), width(0), height(0), logW(0), logH(0), keys(SDL_GetKeyboardState(nullptr)) {}
};

struct Resources {
    const int ANIM_PLAYER_IDLE = 0;
    const int ANIM_PLAYER_WALK = 1;
    vector<Animation> playerAnims;
    vector<SDL_Texture*> textures;
    SDL_Texture* texIdle,* texWalk;

    SDL_Texture* loadTexture(SDL_Renderer* renderer, const string& texPath) {
        //load image
        string imgPath = (string)SDL_GetBasePath() + texPath;
        SDL_Surface* surface = SDL_LoadBMP(imgPath.c_str());
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST); //scale mode for a picture
        textures.push_back(tex);
        return tex;
    }

    Resources():texIdle(nullptr), texWalk(nullptr) {}

    void load(SDLState& state) {
        playerAnims.resize(5);
        //playerAnims[ANIM_PLAYER_IDLE] = Animation(4, 1.6f);
        //playerAnims[ANIM_PLAYER_WALK] = Animation(4, 0.5f);
        playerAnims[ANIM_PLAYER_IDLE] = Animation();
        playerAnims[ANIM_PLAYER_WALK] = Animation();

        texIdle = loadTexture(state.renderer, "Assets/playerIdle.bmp");
        texWalk = loadTexture(state.renderer, "Assets/playerWalk.bmp");
    }

    void unload() {
        for (SDL_Texture* tex : textures)
            SDL_DestroyTexture(tex);
    }
};

struct GameState {
    array<vector<GameObject>,2> layers;
    int playerIndex;

    GameState() :playerIndex(0) {}
};

bool initSDL(SDLState&);
void cleanSDL(SDLState&);
void drawObject(const SDLState&, GameState&, GameObject&, float);
void update(const SDLState&, GameState&, Resources&, GameObject&, float);
void createTiles(SDLState&, GameState&, const Resources&);


int main() {
    SDLState state;
    state.width = 1280;
    state.height = 720;
    state.logW = 640;
    state.logH = 320;
    if (!initSDL(state)) { return 1; }

    //SDL_Texture* fog = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, state.width, state.height); //fog.v1
    //load game assets
    Resources res;
    res.load(state);

    ///setup game
    GameState gs;
    createTiles(state, gs, res);
    uint64_t prevTime = SDL_GetTicks();

    /*//blend mode setup for fog.v1
    SDL_BlendMode carveBlendMode = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ZERO,                // Src Color
        SDL_BLENDFACTOR_ONE,                 // Dst Color
        SDL_BLENDOPERATION_ADD,              // Color Operation
        SDL_BLENDFACTOR_ZERO,                // Src Alpha
        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, // Dst Alpha (Reduces alpha where to draw)
        SDL_BLENDOPERATION_ADD               // Alpha Operation
    );
    SDL_SetTextureBlendMode(fog, SDL_BLENDMODE_BLEND);*/

    //gameloop
    bool running = true;
    while (running) {
        uint64_t nowTime = SDL_GetTicks();
        float deltaTime = (nowTime - prevTime) / 1000.0f;
        SDL_Event event{ 0 };
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
        }

        //update all objects
        for (auto& layer : gs.layers)
            for (GameObject& obj : layer){
                update(state, gs, res, obj, deltaTime);
                if (obj.currentAnimation != -1)
                    obj.animations[obj.currentAnimation].step(deltaTime);
            }

        //prepare render
        SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
        SDL_RenderClear(state.renderer);

        //draw all objects
        for(auto &layer : gs.layers)
            for (GameObject& obj : layer) {
                drawObject(state, gs, obj, deltaTime);
            }

        /*//fog.v1
        SDL_SetRenderTarget(state.renderer, fog);
        SDL_SetRenderDrawColor(state.renderer, 10, 10, 15, 240);
        SDL_RenderClear(state.renderer);

        //carve vision
        SDL_SetRenderDrawBlendMode(state.renderer, carveBlendMode);
        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);

        for (int w = 0; w < vision * 2; w++) {
            for (int h = 0; h < vision * 2; h++) {
                int dx = vision - w;
                int dy = vision - h;
                if ((dx * dx + dy * dy) <= (vision * vision)) {
                    SDL_RenderPoint(state.renderer, playerX + dx, playerY + dy);
                }
            }
        }

        SDL_SetRenderTarget(state.renderer, nullptr);
        SDL_RenderTexture(state.renderer, fog, nullptr, nullptr);*/

        //render
        SDL_RenderPresent(state.renderer);

        prevTime = nowTime;
    }
    //cout<< SDL_GetBasePath() <<endl; // show base path
    res.unload();
    //SDL_DestroyTexture(fog);//fog.v1
    cleanSDL(state);
    return 0;
}

bool initSDL(SDLState& state) {
    bool initSuccess = true;

    if (!SDL_Init(SDL_INIT_VIDEO)) { initSuccess = false; }

    //create window
    state.window = SDL_CreateWindow("Escape Anything", state.width, state.height, SDL_WINDOW_RESIZABLE);
    if (!state.window) { cleanSDL(state); initSuccess = false; }

    //create renderer
    state.renderer = SDL_CreateRenderer(state.window, nullptr);
    if (!state.renderer) { cleanSDL(state); initSuccess = false; }

    //configure presentation
    SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    return initSuccess;
}

void cleanSDL(SDLState& state) {
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}

void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float deltaTime) {
    const float spriteSize = 32;
    float srcX = (obj.currentAnimation != -1) ? obj.animations[obj.currentAnimation].currentFrame() * spriteSize : 0.0f;
    SDL_FRect src{
        src.x = srcX,
        src.y = 0,
        src.w = spriteSize,
        src.h = spriteSize
    };

    SDL_FRect dst{
        dst.x = obj.position.x,
        dst.y = obj.position.y,
        dst.w = spriteSize,
        dst.h = spriteSize
    };

    SDL_RenderTexture(state.renderer, obj.texture, &src, &dst);
    //SDL_RenderTextureRotated(state.renderer, idleTex, &src, &dst, 0, nullptr, (flipHorizontal) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime) {
    if (obj.type == ObjectType::player) {
        //handle player movement
        float moveX, moveY = moveX = 0;

        if (state.keys[SDL_SCANCODE_A]) {
            moveX -= obj.data.player.playerSpeed;
        }
        if (state.keys[SDL_SCANCODE_D]) {
            moveX += obj.data.player.playerSpeed;
        }
        if (state.keys[SDL_SCANCODE_W]) {
            moveY -= obj.data.player.playerSpeed;
        }
        if (state.keys[SDL_SCANCODE_S]) {
            moveY += obj.data.player.playerSpeed;
        }
        obj.position.x += moveX * deltaTime;
        obj.position.y += moveY * deltaTime;

        switch (obj.data.player.state) {
            case PlayerState::idle: {
               if (moveX != 0 || moveY != 0) {
                    obj.data.player.state = PlayerState::walking;
                   obj.texture = res.texWalk;
                   //obj.currentAnimation = res.ANIM_PLAYER_WALK;
                }
                break;
            }
            case PlayerState::walking: {
                if (moveX == 0 && moveY == 0) {
                    obj.data.player.state = PlayerState::idle;
                    obj.texture = res.texIdle;
                    //obj.currentAnimation = res.ANIM_PLAYER_IDLE;
                }
                break;
            }
        }
    }
}

void createTiles(SDLState& state, GameState& gs, const Resources& res) {
    /*
        1 - Ground
        2 - Door
        3 - Enemy
        4 - Player
        5 - Wall
        6 - Items
    */

    short map[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    for(int r = 0; r < MAP_ROWS; r++)
        for(int c = 0; c < MAP_COLS; c++)
            switch (map[r][c]) {
                case 1: {//Ground
                    break;
                }
                case 2: {//Door
                    break;
                }
                case 3: {//Enemy
                    break;
                }
                case 4: {//Player
                    GameObject player;
                    player.position = Vec2((float)c * TILE_SIZE, (float)r * TILE_SIZE);
                    player.type = ObjectType::player;
                    player.data.player = PlayerData();
                    player.texture = res.texIdle;
                    player.animations = res.playerAnims;
                    //player.currentAnimation = res.ANIM_PLAYER_IDLE;
                    gs.layers[LAYER_IDK_CHARACTERS].push_back(player);
                    break;
                }
                case 5: {//Wall
                    break;
                }
                case 6: {//Items
                    break;
                }
            }
}