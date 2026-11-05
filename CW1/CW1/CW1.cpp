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

    //enemy anim
    const int ANIM_ENEMY = 0;
    const int ANIM_ENEMY_DIE = 1;
    vector<Animation> enemyAnims;

    vector<SDL_Texture*> textures;
    SDL_Texture* texIdle,* texWalk,* texGround,* texWall,* texBullet,* texBulletHit,* texEnemy,* texEnemyDie;

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

    Resources():texIdle(nullptr), texWalk(nullptr), texGround(nullptr), texWall(nullptr), texBullet(nullptr),
        texBulletHit(nullptr), texEnemy(nullptr), texEnemyDie(nullptr) {}

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

        //enemy anims
        enemyAnims.resize(2);
        enemyAnims[ANIM_ENEMY] = Animation(3, 1.0f);
        enemyAnims[ANIM_ENEMY_DIE] = Animation(3, 2.0f);

        //load texture from bmp file by path
        texIdle = loadTexture(state.renderer, "Assets/playerIdle.bmp");
        texWalk = loadTexture(state.renderer, "Assets/playerWalk.bmp");
        texGround = loadTexture(state.renderer, "Assets/ground2.bmp");
        texWall = loadTexture(state.renderer, "Assets/wall1.bmp");
        texBullet = loadTexture(state.renderer, "Assets/bullet.bmp");
        texBulletHit = loadTexture(state.renderer, "Assets/bulletHit.bmp");
        texEnemy = loadTexture(state.renderer, "Assets/enemy.bmp");
        texEnemyDie = loadTexture(state.renderer, "Assets/enemyDie.bmp");
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

    //setup game
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
            }

        for (GameObject& bullet : gs.bullets) {
            update(state, gs, res, bullet, deltaTime);
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
    cleanSDL(state);
    return 0;
}

//use for initialize SDL
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
        obj.position.y,
        width,
        height
    };

    if (obj.shouldFlash)
        SDL_SetTextureColorModFloat(obj.texture, 5.0f, 1.0f, 1.0f);
    if (obj.type == ObjectType::level)
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
        float cy = TILE_SIZE / 2 + obj.position.y;
        float dx = mouseX - cx;
        float dy = mouseY - cy;

        //update player angle dynamically if the mouse has moved away from the player center
        if (abs(dx) > 1.0f || abs(dy) > 1.0f) {
            float mouseAngleRad = atan2(dy, dx);
            gs.playerAngle = mouseAngleRad * 180 / 3.14159265f;
        }

        
        //handle player movement
        Vec2 direction;

        if (state.keys[SDL_SCANCODE_A])
            direction.x -= 1;
        if (state.keys[SDL_SCANCODE_D])
            direction.x += 1;
        if (state.keys[SDL_SCANCODE_W])
            direction.y -= 1;
        if (state.keys[SDL_SCANCODE_S])
            direction.y += 1;

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
            float variationX, variationY = variationX = 40;
            variationX = SDL_rand(variationX) - variationX / 2;
            variationY = SDL_rand(variationY) - variationY / 2;
            bullet.direction = Vec2((dx + variationX) / d, (dy + variationY) / d);

            bullet.collider = SDL_FRect{ 0,0,static_cast<float>(res.texBullet->h),static_cast<float>(res.texBullet->h) };
            bullet.position = Vec2(cx + gs.mapViewport.x + (12 * bullet.direction.x), cy + (12 * bullet.direction.y));
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
        Timer& weaponTimer = obj.data.player.weaponTimer;
        weaponTimer.step(deltaTime);
        if (mouseB & SDL_BUTTON_LMASK) {
            if (weaponTimer.isTimeout()) {
                weaponTimer.reset();
                spawnBullet();
            }
        }

        //debug mode
        if (!state.keys[SDL_SCANCODE_F10]&&gs.pre) {
            gs.debugmode = !gs.debugmode;
        }
        gs.pre = state.keys[SDL_SCANCODE_F10];

        //handle player FSM
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
        const auto playerInRange = [&]() {
            //check player nearby or not
            float dx = gs.player().position.x - obj.position.x;
            float dy = gs.player().position.y - obj.position.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (enemydata.state != EnemyState::dead && d < 100) {
                enemydata.state = EnemyState::attack;
                obj.direction = Vec2(dx / d, dy / d);
                obj.velocity = Vec2(80 * obj.direction.x, 80 * obj.direction.y);
                return true;
            }
            return false;
        };

        //handle enemy FSM
        switch (enemydata.state) {
            case EnemyState::idle: {
                if (playerInRange()) {}
                else if (enemydata.idleTimer.step(deltaTime)) {
                    enemydata.state = EnemyState::roaming;
                    int angle = SDL_rand(360);
                    obj.direction = Vec2(cos(angle), sin(angle));
                    obj.velocity = Vec2(80 * obj.direction.x, 80 * obj.direction.y);
                }
                break;
            }
            case EnemyState::roaming: {
                if (playerInRange()) {}
                else if (enemydata.roamingTimer.step(deltaTime)) {
                    enemydata.state = EnemyState::idle;
                    obj.velocity = Vec2(0,0);
                }
                break;
            }
            case EnemyState::attack: {
                enemydata.cooldownTimer.step(deltaTime);
                if (enemydata.cooldownTimer.isTimeout()) {
                    obj.velocity = Vec2(80 * obj.direction.x, 80 * obj.direction.y);
                    if (playerInRange())
                        enemydata.attackTimer.reset();
                    else if (enemydata.attackTimer.step(deltaTime)) {
                        enemydata.state = EnemyState::idle;
                        obj.velocity = Vec2(0, 0);
                    }
                }
                break;
            }
            case EnemyState::damaged: {
                if (enemydata.damagedTimer.step(deltaTime)) {
                    if (!playerInRange())
                        enemydata.state = EnemyState::idle;
                }
                break;
            }
            case EnemyState::dead: {
                if (obj.currentAnimation != -1 && obj.animations[obj.currentAnimation].isDone()) {
                    obj.currentAnimation = -1;
                    obj.spriteFrame = 3;
                }
                break;
            }
        }
    }

    //update Game Object location
    obj.position += obj.velocity * deltaTime;

    //check the Game Object collide with other Game Object or not
    for (auto& layer : gs.layers) {
        for (GameObject& objB : layer) {
            if (&obj != &objB) {
                checkCollision(state,gs,res,obj,objB,deltaTime);
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
                if (objB.data.enemy.state != EnemyState::dead) {
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
                            objB.shouldFlash = true;
                            objB.flashTimer.reset();
                            enemydata.state = EnemyState::damaged;
                            enemydata.healthPoints -= 10;
                            if (enemydata.healthPoints <= 0) {
                                enemydata.state = EnemyState::dead;
                                objB.texture = res.texEnemyDie;
                                objB.currentAnimation = res.ANIM_ENEMY_DIE;
                                objB.velocity = Vec2(0, 0);
                            }
                        }
                        else {
                            passthrough = true;
                        }
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
        if (objB.type == ObjectType::player) {
            objA.data.enemy.cooldownTimer.reset();
            objA.velocity = Vec2(0, 0);
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
        2 - Door
        3 - Enemy
        4 - Player
        5 - Wall
        6 - Items
    */

    //the map
    short map[MAP_ROWS][MAP_COLS] = {
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 0, 0, 0, 0, 0, 0, 3, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 0, 0, 0, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    //background map
    short backgroundMap[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

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
                    case 2: {//Door
                        break;
                    }
                    case 3: {//Enemy
                        GameObject enemy = createObject(r, c, res.texEnemy, ObjectType::enemy);
                        enemy.data.enemy = EnemyData();
                        enemy.currentAnimation = res.ANIM_ENEMY;
                        enemy.animations = res.enemyAnims;
                        enemy.acceleration = 300;
                        enemy.maxSpeed = 80;
                        enemy.collider = { 0,0,32,32 };
                        gs.layers[LAYER_IDK_CHARACTERS].push_back(enemy);
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
    loadMap(backgroundMap);
    //debug use
    assert(gs.playerIndex != -1);
}

//Bresenham's Line Trace for Line of Sight
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
        //stop when outside the fog map
        if (cx < 0 || cx >= FOG_COLS || cy < 0 || cy >= FOG_ROWS) break;
        gs.fogGrid[cy][cx] = FogState::Visible;
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

    //player centre always visible
    if(cy >= 0 && cy < FOG_ROWS && cx >= 0 && cx < FOG_COLS)
        gs.fogGrid[cy][cx] = FogState::Visible;
}