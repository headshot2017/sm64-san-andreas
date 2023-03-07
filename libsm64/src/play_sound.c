#include "play_sound.h"

#include "decomp/audio/external.h"
#include "load_audio_data.h"

SM64PlaySoundFunctionPtr g_play_sound_func = NULL;

extern void play_sound( uint32_t soundBits, f32 *pos ) {
    if ( g_is_audio_initialized ) {
        sSoundRequests[sSoundRequestCount].soundBits = soundBits;
        sSoundRequests[sSoundRequestCount].position = pos;
        sSoundRequestCount++;
    }

    if ( g_play_sound_func ) {
        g_play_sound_func(soundBits, pos);
    }
}
