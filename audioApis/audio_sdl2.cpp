#include "audio_sdl2.h"

#include <fstream>

bool AudioAPI_SDL2::init()
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        std::ofstream log("libsm64.log", std::ios::app);
        log << "SDL_Init(SDL_INIT_AUDIO) failure: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 32000;
    want.format = AUDIO_S16;
    want.channels = 2;
    want.samples = 512;
    want.callback = NULL;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) {
        std::ofstream log("libsm64.log", std::ios::app);
        log << "SDL_OpenAudioDevice error: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_PauseAudioDevice(dev, 0);
    return true;
}

bool AudioAPI_SDL2::destroy()
{
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return true;
}

void AudioAPI_SDL2::play(int16_t* audioBuffer, uint32_t numSamples)
{
    if (getBuffered() < 6000)
        SDL_QueueAudio(dev, audioBuffer, numSamples);
}

int AudioAPI_SDL2::getBuffered()
{
    return SDL_GetQueuedAudioSize(dev) / 4;
}
