//#include "SDL3/SDL_main.h"
#define NDEBUG 
#include "assert.h"
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
    //player anim
    const int ANIM_PLAYER_IDLE = 0;
    const int ANIM_PLAYER_WALK = 1;
    vector<Animation> playerAnims;

    //bullet anim
    const int ANIM_BULLET_MOVING = 0;
    const int ANIM_BULLET_HIT = 1;
    vector<Animation> bulletAnims;

    vector<SDL_Texture*> textures;
    SDL_Texture* texIdle,* texWalk,* texGround,* texWall,* texBullet,* texBulletHit;

    SDL_Texture* loadTexture(SDL_Renderer* renderer, const string& texPath) {
        //load image
        string imgPath = (string)SDL_GetBasePath() + texPath;
        SDL_Surface* surface = SDL_LoadBMP(imgPath.c_str());

        //remove white color as remove background
        Uint32 colorkey = SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format),nullptr, 255, 255, 255);
        SDL_SetSurfaceColorKey(surface, true, colorkey);

        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST); //scale mode for a picture
        textures.push_back(tex);
        return tex;
    }

    Resources():texIdle(nullptr), texWalk(nullptr), texGround(nullptr), texWall(nullptr), texBullet(nullptr),
        texBulletHit(nullptr) {}

    void load(SDLState& state) {
        //player anims
        playerAnims.resize(5);
        //playerAnims[ANIM_PLAYER_IDLE] = Animation(4, 1.6f);
        //playerAnims[ANIM_PLAYER_WALK] = Animation(4, 0.5f);
        playerAnims[ANIM_PLAYER_IDLE] = Animation();
        playerAnims[ANIM_PLAYER_WALK] = Animation();

        //bullet anims
        bulletAnims.resize(2);
        bulletAnims[ANIM_BULLET_MOVING] = Animation(3, 0.05f);
        bulletAnims[ANIM_BULLET_HIT] = Animation(4, 0.15f);

        texIdle = loadTexture(state.renderer, "Assets/playerIdle.bmp");
        texWalk = loadTexture(state.renderer, "Assets/playerWalk.bmp");
        texGround = loadTexture(state.renderer, "Assets/ground2.bmp");
        texWall = loadTexture(state.renderer, "Assets/wall1.bmp");
        texBullet = loadTexture(state.renderer, "Assets/bullet.bmp");
        texBulletHit = loadTexture(state.renderer, "Assets/bulletHit.bmp");
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

    array<vector<GameObject>,2> layers;
    vector<GameObject> background, foreground, bullets;
    int playerIndex;
    float playerAngle;
    bool debugmode,pre;

    GameState(const SDLState& state) :playerIndex(-1), playerAngle(0), fogGrid(FOG_ROWS, vector<FogState>(FOG_COLS, FogState::Unexplored)),
        wallGrid(FOG_ROWS, vector<bool>(FOG_COLS, 0)), mapViewport(SDL_FRect{0,0,static_cast<float>(state.logW),static_cast<float>(state.logH)}),
    debugmode(false),pre(false){}
    GameObject& player() { return layers[LAYER_IDK_CHARACTERS][playerIndex]; }
};

bool initSDL(SDLState&);
void cleanSDL(SDLState&);
void drawObject(const SDLState&, GameState&, GameObject&,float, float, float);
void update(const SDLState&, GameState&, Resources&, GameObject&, float);
void createTiles(SDLState&, GameState&, const Resources&);
void CastVisionRay(int, int, int, int, GameState&);
void fogHandle(GameState&);
void collisionResponse(const SDLState&, GameState&, Resources&, const SDL_FRect&, const SDL_FRect&, const SDL_FRect&, GameObject&, GameObject&, float);
void checkCollision(const SDLState&, GameState&, Resources&, GameObject&, GameObject&, float);


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
    GameState gs(state);
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

        for (GameObject& bullet : gs.bullets) {
            update(state, gs, res, bullet, deltaTime);
            if (bullet.currentAnimation != -1)
                bullet.animations[bullet.currentAnimation].step(deltaTime);
        }

        fogHandle(gs);

        //calculatte viewport
        gs.mapViewport.x = (gs.player().position.x + TILE_SIZE / 2) - gs.mapViewport.w / 2;

        //prepare render
        SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
        SDL_RenderClear(state.renderer);

        //draw background
        for (GameObject& obj : gs.background) {
            SDL_FRect dst{
                obj.position.x - gs.mapViewport.x,
                obj.position.y,
                static_cast<float>(obj.texture->w),
                static_cast<float>(obj.texture->h)
            };
            SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
        }

        //draw all objects
        for(auto &layer : gs.layers)
            for (GameObject& obj : layer) {
                drawObject(state, gs, obj, TILE_SIZE, TILE_SIZE, deltaTime);
            }

        //draw bullets
        for (GameObject& bullet : gs.bullets) {
            if(bullet.data.bullet.state != BulletState::inactive)
                drawObject(state, gs, bullet, bullet.collider.w, bullet.collider.h, deltaTime);
        }

        //draw foreground
        for (GameObject& obj : gs.foreground) {
            SDL_FRect dst{
                obj.position.x - gs.mapViewport.x,
                obj.position.y,
                static_cast<float>(obj.texture->w),
                static_cast<float>(obj.texture->h)
            };
            SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
        }

        //fog
        SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);

        for (int y = 0; y < FOG_ROWS; ++y)
            for (int x = 0; x < FOG_COLS; ++x){
                SDL_FRect fogRect = { (float)x - gs.mapViewport.x, (float)y, 1.0f, 1.0f };

                if (gs.fogGrid[y][x] == FogState::Unexplored) {
                    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255); // Shroud
                    SDL_RenderFillRect(state.renderer, &fogRect);
                }
                else if (gs.fogGrid[y][x] == FogState::Explored) {
                    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 165); // Fog
                    SDL_RenderFillRect(state.renderer, &fogRect);
                }
            }

        //debug use info
        if (gs.debugmode) {
            string msg = "S: " + to_string(static_cast<int>(gs.player().data.player.state)) + ", B: " + to_string(gs.bullets.size());
            SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(state.renderer, 5, 5, msg.c_str());
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
    SDL_SetRenderVSync(state.renderer,1);

    //configure presentation
    SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    return initSuccess;
}

void cleanSDL(SDLState& state) {
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}

void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float width, float height, float deltaTime) {
    float srcX = (obj.currentAnimation != -1) ? obj.animations[obj.currentAnimation].currentFrame() * width : 0.0f;
    SDL_FRect src{
        srcX,
        0,
        width,
        height
    };

    SDL_FRect dst{
        obj.position.x - gs.mapViewport.x,
        obj.position.y,
        width,
        height
    };

    SDL_RenderTexture(state.renderer, obj.texture, &src, &dst);
    //SDL_RenderTextureRotated(state.renderer, idleTex, &src, &dst, 0, nullptr, (flipHorizontal) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

    if (gs.debugmode) {
        SDL_FRect rectA{
        obj.position.x + obj.collider.x - gs.mapViewport.x,
        obj.position.y + obj.collider.y,
        obj.collider.w,
        obj.collider.h
        };
        SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 150);
        SDL_RenderFillRect(state.renderer, &rectA);
        SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);
    }
}

void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime) {
    if (obj.type == ObjectType::player) {
        //handle player mouse inputs
        float rawMouseX, rawMouseY, mouseX, mouseY;
        SDL_MouseButtonFlags mouseB = SDL_GetMouseState(&rawMouseX, &rawMouseY);        
        SDL_RenderCoordinatesFromWindow(state.renderer, rawMouseX, rawMouseY, &mouseX, &mouseY);

        //handle player vision
        float cx = TILE_SIZE / 2 + obj.position.x - gs.mapViewport.x;
        float cy = TILE_SIZE / 2 + obj.position.y;
        float dx = mouseX - cx;
        float dy = mouseY - cy;

        // Update player angle dynamically if the mouse has moved away from the player center
        if (abs(dx) > 1.0f || abs(dy) > 1.0f) {
            float mouseAngleRad = atan2(dy, dx);
            gs.playerAngle = mouseAngleRad * 180 / 3.14159265f;
        }

        
        //handle player movement
        Vec2 direction;

        if (state.keys[SDL_SCANCODE_A]) {
            direction.x -= 1;
        }
        if (state.keys[SDL_SCANCODE_D]) {
            direction.x += 1;
        }
        if (state.keys[SDL_SCANCODE_W]) {
            direction.y -= 1;
        }
        if (state.keys[SDL_SCANCODE_S]) {
            direction.y += 1;
        }
        if (direction.x) {
            obj.direction.x = direction.x;
        }
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
        if (direction.y) {
            obj.direction.y = direction.y;
        }
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
        Timer& weaponTimer = obj.data.player.weaponTimer;
        weaponTimer.step(deltaTime);
        if (mouseB & SDL_BUTTON_LMASK) {
            if (weaponTimer.isTimeout()) {
                weaponTimer.reset();
                //spawn bullet
                GameObject bullet;
                float d = sqrtf(dx * dx + dy * dy);
                bullet.data.bullet = BulletData();
                bullet.type = ObjectType::bullet;
                float variationX, variationY = variationX = 40;
                variationX = SDL_rand(variationX) - variationX / 2;
                variationY = SDL_rand(variationY) - variationY / 2;
                bullet.direction = Vec2((dx + variationX) / d, (dy + variationY) / d);
                bullet.texture = res.texBullet;
                bullet.currentAnimation = res.ANIM_BULLET_MOVING;
                bullet.collider = SDL_FRect{ 0,0,static_cast<float>(res.texBullet->h),static_cast<float>(res.texBullet->h) };
                bullet.velocity = Vec2(obj.velocity.x + 200 * bullet.direction.x, obj.velocity.y + 200 * bullet.direction.y);
                bullet.animations = res.bulletAnims;
                bullet.position = Vec2(cx + gs.mapViewport.x + (12 * bullet.direction.x), cy + (12 * bullet.direction.y));
                
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
            }
        }
        if (!state.keys[SDL_SCANCODE_F10]&&gs.pre) {
            gs.debugmode = !gs.debugmode;
        }
        gs.pre = state.keys[SDL_SCANCODE_F10];

        switch (obj.data.player.state) {
            case PlayerState::idle: {
                if (direction.x != 0 || direction.y != 0) {
                    obj.data.player.state = PlayerState::walking;
                }
                obj.texture = res.texIdle;
                //obj.currentAnimation = res.ANIM_PLAYER_IDLE;
                break;
            }
            case PlayerState::walking: {
                if (direction.x == 0 && direction.y == 0) {
                    obj.data.player.state = PlayerState::idle;
                }
                obj.texture = res.texWalk;
                //obj.currentAnimation = res.ANIM_PLAYER_WALK;
                break;
            }
        }

        obj.velocity += direction * obj.acceleration * deltaTime;
        if (abs(obj.velocity.x) > obj.maxSpeed)
            obj.velocity.x = direction.x * obj.maxSpeed;
        if (abs(obj.velocity.y) > obj.maxSpeed)
            obj.velocity.y = direction.y * obj.maxSpeed;
    }
    else if (obj.type == ObjectType::bullet) {
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

    obj.position += obj.velocity * deltaTime;

    for (auto& layer : gs.layers) {
        for (GameObject& objB : layer) {
            if (&obj != &objB) {
                checkCollision(state,gs,res,obj,objB,deltaTime);
            }
        }
    }
}

void collisionResponse(const SDLState& state, GameState& gs, Resources& res, const SDL_FRect& rectA, const SDL_FRect& rectB, const SDL_FRect& rectC,
    GameObject& objA, GameObject& objB, float deltaTime) {
    const auto genericResponse = [&]() {
        if (rectC.w < rectC.h) {
            //horizontal collision
            objA.position.x += (objA.velocity.x > 0) ? -rectC.w : rectC.w;
            objA.velocity.x = 0;
        }
        else {
            //vertical collision
            objA.position.y += (objA.velocity.y > 0) ? -rectC.h : rectC.h;
            objA.velocity.y = 0;
        }
    };
    if (objA.type == ObjectType::player) {
        switch (objB.type) {
            case ObjectType::level: {
                genericResponse();
                break;
            }
        }
    }
    else if (objA.type == ObjectType::bullet) {
        switch (objA.data.bullet.state) {
            case BulletState::moving: {
                genericResponse();
                objA.velocity *= 0;
                objA.data.bullet.state = BulletState::colliding;
                objA.texture = res.texBulletHit;
                objA.currentAnimation = res.ANIM_BULLET_HIT;
                break;
            }
        }
    }
}

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

    if (SDL_GetRectIntersectionFloat(&rectA,&rectB,&rectC)) {
        //intersection
        collisionResponse(state,gs,res,rectA,rectB,rectC,a,b,deltaTime);
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
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 0, 0, 0, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    short foregroundMap[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    short backgroundMap[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    const auto loadMap = [&state, &gs, &res](short layer[MAP_ROWS][MAP_COLS]) {
        const auto createObject = [&state](int r, int c, SDL_Texture* tex, ObjectType type) {
            GameObject o;
            o.type = type;
            o.position = Vec2(c * TILE_SIZE, r * TILE_SIZE);
            o.texture = tex;
            o.collider = { 0,0,TILE_SIZE,TILE_SIZE };
            return o;
        };

        for (int r = 0; r < MAP_ROWS; r++)
            for (int c = 0; c < MAP_COLS; c++)
                switch (layer[r][c]) {
                    case 1: {//Ground
                        GameObject ground = createObject(r, c, res.texGround, ObjectType::level);
                        gs.background.push_back(ground);
                        break;
                    }
                    case 2: {//Door
                        break;
                    }
                    case 3: {//Enemy
                        break;
                    }
                    case 4: {//Player
                        GameObject player = createObject(r, c, res.texIdle, ObjectType::player);
                        player.data.player = PlayerData();
                        player.animations = res.playerAnims;
                        player.acceleration = 300;
                        player.maxSpeed = 100;
                        player.collider = { 11,6,10,20 };
                        //player.currentAnimation = res.ANIM_PLAYER_IDLE;
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
                        break;
                    }
                }
    };
    loadMap(map);
    loadMap(foregroundMap);
    loadMap(backgroundMap);
    assert(gs.playerIndex != -1);
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

void fogHandle(GameState& gs) {
    const GameObject& obj = gs.player();
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