#pragma once
#include <cstdint>
#include <cstddef>
typedef uint8_t Uint8; typedef uint64_t Uint64; typedef uint32_t Uint32;
struct SDL_Renderer; struct SDL_Texture;
struct SDL_FRect { float x,y,w,h; }; struct SDL_FPoint { float x,y; };
enum SDL_FlipMode { SDL_FLIP_NONE };
enum { SDL_EVENT_KEY_DOWN=1, SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_BUTTON_UP };
enum { SDLK_UP, SDLK_W, SDLK_DOWN, SDLK_S, SDLK_RETURN, SDLK_SPACE, SDLK_ESCAPE };
enum { SDL_BUTTON_LEFT=1 };
enum { SDL_BLENDMODE_NONE, SDL_BLENDMODE_BLEND };
#define SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE 8
struct SDL_KeyboardEvent { Uint32 type; bool repeat; int key; };
struct SDL_MouseMotionEvent { Uint32 type; float x,y; };
struct SDL_MouseButtonEvent { Uint32 type; Uint8 button; float x,y; };
union SDL_Event { Uint32 type; SDL_KeyboardEvent key; SDL_MouseMotionEvent motion; SDL_MouseButtonEvent button; };
inline Uint64& fakeTicks(){static Uint64 t=1000;return t;}
inline Uint64 SDL_GetTicks(){return fakeTicks();}
inline int& putCalls(){static int n=0;return n;}
inline bool SDL_PointInRectFloat(const SDL_FPoint*p,const SDL_FRect*r){return p->x>=r->x&&p->y>=r->y&&p->x<r->x+r->w&&p->y<r->y+r->h;}
inline bool SDL_ConvertEventToRenderCoordinates(SDL_Renderer*,SDL_Event*){return true;}
inline bool SDL_RenderTexture(SDL_Renderer*,SDL_Texture*,const SDL_FRect*,const SDL_FRect*){return true;}
inline bool SDL_RenderTextureRotated(SDL_Renderer*,SDL_Texture*,const SDL_FRect*,const SDL_FRect*,double,const SDL_FPoint*,SDL_FlipMode){return true;}
inline bool SDL_SetRenderDrawColor(SDL_Renderer*,Uint8,Uint8,Uint8,Uint8){return true;}
inline bool SDL_SetRenderDrawBlendMode(SDL_Renderer*,int){return true;}
inline bool SDL_RenderFillRect(SDL_Renderer*,const SDL_FRect*){return true;}
inline bool SDL_RenderDebugText(SDL_Renderer*,float,float,const char*){return true;}
typedef uint32_t SDL_AudioDeviceID; struct SDL_AudioStream;
typedef int SDL_AudioFormat; enum { SDL_AUDIO_S16 = 0x8010 };
struct SDL_AudioSpec { SDL_AudioFormat format; int channels; int freq; };
#define SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK ((SDL_AudioDeviceID)0xFFFFFFFFu)
inline SDL_AudioDeviceID SDL_OpenAudioDevice(SDL_AudioDeviceID, const SDL_AudioSpec*){return 1;}
inline void SDL_CloseAudioDevice(SDL_AudioDeviceID){}
inline SDL_AudioStream* SDL_CreateAudioStream(const SDL_AudioSpec*, const SDL_AudioSpec*){static int d[64]; static int i=0; return reinterpret_cast<SDL_AudioStream*>(&d[i++]);}
inline bool SDL_BindAudioStream(SDL_AudioDeviceID, SDL_AudioStream*){return true;}
inline void SDL_DestroyAudioStream(SDL_AudioStream*){}
inline const char* SDL_GetBasePath(){return "";}
inline bool SDL_LoadWAV(const char*, SDL_AudioSpec* sp, Uint8** b, Uint32* l){static Uint8 buf[88200]; *b=buf; *l=88200; *sp=SDL_AudioSpec{SDL_AUDIO_S16,1,44100}; return true;}
inline void SDL_free(void*){}
inline int SDL_GetAudioStreamQueued(SDL_AudioStream*){return 0;}
inline bool SDL_SetAudioStreamGain(SDL_AudioStream*, float){return true;}
inline bool SDL_SetAudioStreamFrequencyRatio(SDL_AudioStream*, float){return true;}
inline bool SDL_PutAudioStreamData(SDL_AudioStream*, const void*, int){putCalls()++; return true;}
