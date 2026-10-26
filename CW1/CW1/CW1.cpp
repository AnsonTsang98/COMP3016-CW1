#include "SDL3/SDL.h"
//#include "SDL3/SDL_main.h"

#include <iostream>
#include <string>
#include <vector>

struct SDLState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    int width, height, logW, logH;
};

bool initSDL(SDLState&);
void cleanSDL(SDLState&);

using namespace std;
int main() {
    SDLState state;
    state.width = 1280;
    state.height = 720;
    state.logW = 640;
    state.logH = 320;
    if (!initSDL(state)) { return 1; }

    //load image
    string imgPath = (string)SDL_GetBasePath() + "Assets/img.bmp";
    SDL_Surface* surface = SDL_LoadBMP(imgPath.c_str());
    SDL_Texture* idleTex = SDL_CreateTextureFromSurface(state.renderer,surface);
    SDL_DestroySurface(surface);
    SDL_SetTextureScaleMode(idleTex, SDL_SCALEMODE_NEAREST); //scale mode for a picture

    ///setup game
    const bool* keys = SDL_GetKeyboardState(nullptr);
    float playerX = 20;
    float playerY = 10;
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

        //handle player movement
        float moveX, moveY = moveX = 0;
        if (keys[SDL_SCANCODE_A]) {
            moveX -= 75.0f;
        }
        if (keys[SDL_SCANCODE_D]) {
            moveX += 75.0f;
        }
        if (keys[SDL_SCANCODE_W]) {
            moveY -= 75.0f;
        }
        if (keys[SDL_SCANCODE_S]) {
            moveY += 75.0f;
        }
        playerX += moveX * deltaTime;
        playerY += moveY * deltaTime;

        //prepare render
        SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
        SDL_RenderClear(state.renderer);

        const float spriteSize = 32;
        SDL_FRect src{
            src.x = 0,
            src.y = 0,
            src.w = spriteSize,
            src.h = spriteSize
        };

        SDL_FRect dst{
            dst.x = playerX,
            dst.y = playerY,
            dst.w = spriteSize,
            dst.h = spriteSize
        };

        SDL_RenderTexture(state.renderer, idleTex, &src, &dst);
        //SDL_RenderTextureRotated(state.renderer, idleTex, &src, &dst, 0, nullptr, (flipHorizontal) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

        //render
        SDL_RenderPresent(state.renderer);

        prevTime = nowTime;
    }
    //cout<< SDL_GetBasePath() <<endl; // show base path
    SDL_DestroyTexture(idleTex);
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