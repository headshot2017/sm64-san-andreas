#include "audio.h"

#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include <libsm64.h>

#include "config.h"
#include "audioApis/audio_sdl2.h"
#include "audioApis/audio_wasapi.h"

pthread_t gSoundThread;
bool audio_started;
AudioAPI* audio_api;

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
		uint32_t numSamples = sm64_audio_tick(audio_api->getBuffered(), 1100, audioBuffer);
		audio_api->play(audioBuffer, numSamples*2*4);

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

    if (getConfig("use_wasapi_audio"))
        audio_api = new AudioAPI_WASAPI();
    else
        audio_api = new AudioAPI_SDL2();

    if (!audio_api->init())
    {
        delete audio_api;
        return;
    }

    // it's best to run audio in a separate thread
    pthread_create(&gSoundThread, NULL, audio_thread, &audio_started);
    audio_started = true;
}

void audio_thread_stop()
{
    if (!audio_started) return;

    pthread_cancel(gSoundThread);
    audio_started = false;

    audio_api->destroy();
    delete audio_api;
}
