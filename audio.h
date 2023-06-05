#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include <inttypes.h>

class AudioAPI
{
public:
    AudioAPI() {}
    ~AudioAPI() {}

    virtual bool init() {return false;}
    virtual bool destroy() {return false;}
    virtual void play(int16_t* audioBuffer, uint32_t numSamples) {}

    virtual int getBuffered() {return 0;}
};

void audio_thread_init();
void audio_thread_stop();

#endif // AUDIO_H_INCLUDED
