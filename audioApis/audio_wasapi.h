#ifndef AUDIO_WASAPI_H_INCLUDED
#define AUDIO_WASAPI_H_INCLUDED

#include "../audio.h"

class AudioAPI_WASAPI : public AudioAPI
{
public:
    bool init();
    bool destroy();
    void play(int16_t* audioBuffer, uint32_t numSamples);

    int getBuffered();

private:
    bool setupStream();
};

#endif // AUDIO_WASAPI_H_INCLUDED
