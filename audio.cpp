#include "audio.h"

#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include <SDL2/SDL.h>
extern "C" {
    #include <libsm64.h>
}

static SDL_AudioDeviceID dev;
pthread_t gSoundThread;
bool audio_started;

long long timeInMilliseconds(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return(((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

void* audio_thread(void* keepAlive)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

	long long currentTime = timeInMilliseconds();
	long long targetTime = 0;
	while(1)
	{
		if(!*((bool*)keepAlive)) return NULL;

		int16_t audioBuffer[544 * 2 * 2];
		uint32_t numSamples = sm64_audio_tick(SDL_GetQueuedAudioSize(dev)/4, 1100, audioBuffer);
		if (SDL_GetQueuedAudioSize(dev)/4 < 6000)
			SDL_QueueAudio(dev, audioBuffer, numSamples * 2 * 4);

		targetTime = currentTime + 33;
		while (timeInMilliseconds() < targetTime)
		{
			usleep(100);
			if(!*((bool*)keepAlive)) return NULL;
		}
		currentTime = timeInMilliseconds();
	}
}

void audio_thread_init()
{
    if (audio_started) return;

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "SDL_INIT_AUDIO failure: %s\n", SDL_GetError());
        return;
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
        fprintf(stderr, "SDL_OpenAudio error: %s\n", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(dev, 0);

    // it's best to run audio in a separate thread
    pthread_create(&gSoundThread, NULL, audio_thread, &audio_started);
    audio_started = true;
}

void audio_thread_stop()
{
    if (!audio_started) return;

    pthread_cancel(gSoundThread);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audio_started = false;
}
