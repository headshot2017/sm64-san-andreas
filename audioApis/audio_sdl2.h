#ifndef AUDIO_SDL2_H_INCLUDED
#define AUDIO_SDL2_H_INCLUDED

#include "../audio.h"
#include <SDL2/SDL.h>

class AudioAPI_SDL2 : public AudioAPI
{
public:
    bool init();
    bool destroy();
    void play(int16_t* audioBuffer, uint32_t numSamples);

    int getBuffered();

private:
    SDL_AudioDeviceID dev;
};

#endif // AUDIO_SDL2_H_INCLUDED
