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
const int FOG_ROWS = MAP_ROWS * TILE_SIZE;
const int FOG_COLS = MAP_COLS * TILE_SIZE;

enum class FogState {
    Unexplored,
    Explored,
    Visible
};

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
    SDL_Texture* texIdle,* texWalk,* texGround,* texWall;

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

    Resources():texIdle(nullptr), texWalk(nullptr), texGround(nullptr), texWall(nullptr){}

    void load(SDLState& state) {
        playerAnims.resize(5);
        //playerAnims[ANIM_PLAYER_IDLE] = Animation(4, 1.6f);
        //playerAnims[ANIM_PLAYER_WALK] = Animation(4, 0.5f);
        playerAnims[ANIM_PLAYER_IDLE] = Animation();
        playerAnims[ANIM_PLAYER_WALK] = Animation();

        texIdle = loadTexture(state.renderer, "Assets/playerIdle.bmp");
        texWalk = loadTexture(state.renderer, "Assets/playerWalk.bmp");
        texGround = loadTexture(state.renderer, "Assets/ground2.bmp");
        texWall = loadTexture(state.renderer, "Assets/wall1.bmp");
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

    array<vector<GameObject>,2> layers;
    int playerIndex;
    float playerAngle;

    GameState() :playerIndex(0), playerAngle(0), fogGrid(FOG_ROWS, vector<FogState>(FOG_COLS, FogState::Unexplored)), wallGrid(FOG_ROWS, vector<bool>(FOG_COLS, 0)) {}
};

bool initSDL(SDLState&);
void cleanSDL(SDLState&);
void drawObject(const SDLState&, GameState&, GameObject&, float);
void update(const SDLState&, GameState&, Resources&, GameObject&, float);
void createTiles(SDLState&, GameState&, const Resources&);
void CastVisionRay(int, int, int, int, GameState&);
void fogHandle(GameState&,const GameObject&);


int main() {
    SDLState state;
    state.width = 1280;
    state.height = 720;
    state.logW = 640;
    state.logH = 320;
    if (!initSDL(state)) { return 1; }

    //load game assets
    Resources res;
    res.load(state);

    ///setup game
    GameState gs;
    createTiles(state, gs, res);
    uint64_t prevTime = SDL_GetTicks();

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

        //fog
        SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);

        for (int y = 0; y < FOG_ROWS; ++y)
            for (int x = 0; x < FOG_COLS; ++x){
                SDL_FRect fogRect = { (float)x, (float)y, 1.0f, 1.0f };

                if (gs.fogGrid[y][x] == FogState::Unexplored) {
                    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255); // Shroud
                    SDL_RenderFillRect(state.renderer, &fogRect);
                }
                else if (gs.fogGrid[y][x] == FogState::Explored) {
                    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 165); // Fog
                    SDL_RenderFillRect(state.renderer, &fogRect);
                }
            }

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
        //handle player vision
        float rawMouseX, rawMouseY, mouseX, mouseY;
        SDL_GetMouseState(&rawMouseX, &rawMouseY);
        SDL_RenderCoordinatesFromWindow(state.renderer, rawMouseX, rawMouseY, &mouseX, &mouseY);

        float cx = TILE_SIZE / 2 + obj.position.x;
        float cy = TILE_SIZE / 2 + obj.position.y;
        float dx = mouseX - cx;
        float dy = mouseY - cy;

        // Update player angle dynamically if the mouse has moved away from the player center
        if (abs(dx) > 1.0f || abs(dy) > 1.0f) {
            float mouseAngleRad = atan2(dy, dx);
            gs.playerAngle = mouseAngleRad * 180 / 3.14159265;
        }

        fogHandle(gs, obj);
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
        0, 4, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 1, 1, 1, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 1, 1, 1, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            switch (map[r][c]) {
                case 1: {//Ground
                    GameObject ground;
                    ground.position = Vec2(c * TILE_SIZE, r * TILE_SIZE);
                    ground.texture = res.texGround;
                    gs.layers[LAYER_IDX_LEVEL].push_back(ground);
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
                    player.position = Vec2(c * TILE_SIZE, r * TILE_SIZE);
                    player.type = ObjectType::player;
                    player.data.player = PlayerData();
                    player.texture = res.texIdle;
                    player.animations = res.playerAnims;
                    //player.currentAnimation = res.ANIM_PLAYER_IDLE;
                    gs.layers[LAYER_IDK_CHARACTERS].push_back(player);
                    break;
                }
                case 5: {//Wall
                    GameObject wall;
                    wall.position = Vec2(c * TILE_SIZE, r * TILE_SIZE);
                    wall.texture = res.texWall;
                    gs.layers[LAYER_IDX_LEVEL].push_back(wall);
                    for (int i = 0; i < TILE_SIZE; i++)
                        for (int k = 0; k < TILE_SIZE; k++)
                            gs.wallGrid[r * TILE_SIZE + i][c * TILE_SIZE + k] = 1;
                    break;
                }
                case 6: {//Items
                    break;
                }
            }
}

// Bresenham's Line Trace for Line of Sight
void CastVisionRay(int x0, int y0, int x1, int y1, GameState& gs) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int cx = x0;
    int cy = y0;
    int wv = 8;

    while (true) {
        if (cx < 0 || cx >= FOG_COLS || cy < 0 || cy >= FOG_ROWS) break;
        gs.fogGrid[cy][cx] = FogState::Visible;
        if (wv <= 0) break;
        if (gs.wallGrid[cy][cx]) wv--;
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

void fogHandle(GameState& gs,const GameObject& obj) {
    //reset fog state
    for (int y = 0; y < FOG_ROWS; y++)
        for (int x = 0; x < FOG_COLS; x++)
            if (gs.fogGrid[y][x] == FogState::Visible)
                gs.fogGrid[y][x] = FogState::Explored;

    //cast rays in limited sector
    int playerVision = obj.data.player.playerVision * TILE_SIZE / 2;
    int steps = obj.data.player.playerFov * 2;
    int cx = TILE_SIZE / 2 + obj.position.x;
    int cy = TILE_SIZE / 2 + obj.position.y;

    for (int i = 0; i < steps; i++) {
        float angle = obj.data.player.playerFov * i / steps + gs.playerAngle - (obj.data.player.playerFov / 2);
        float rad = angle * 3.14159265 / 180;

        int targetX = cos(rad) * playerVision + cx;
        int targetY = sin(rad) * playerVision + cy;
        CastVisionRay(cx, cy, targetX, targetY, gs);
    }

    gs.fogGrid[cy][cx] = FogState::Visible;
}