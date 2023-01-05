#include "plugin.h"
#include "CHud.h"

#include "audio.h"
extern "C" {
    #include <libsm64.h>
    #include <decomp/include/PR/ultratypes.h>
    #include <decomp/include/audio_defines.h>
}

using namespace plugin;

bool loaded = false;
std::string message;

class sm64_san_andreas {
public:
    static void init()
    {
        std::ifstream file("sm64.us.z64", std::ios::ate | std::ios::binary);

        if (!file)
        {
            message = "Super Mario 64 US ROM not found!\nPlease provide a ROM with the filename \"sm64.us.z64\"";
        }
        else
        {
            // load ROM into memory
            uint8_t *romBuffer;
            size_t romFileLength = file.tellg();

            romBuffer = new uint8_t[romFileLength + 1];
            file.seekg(0);
            file.read((char*)romBuffer, romFileLength);
            romBuffer[romFileLength] = 0;
            file.close();

            // Mario texture is 704x64 RGBA
            uint8_t* mario_texture = (uint8_t*)malloc(4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT);

            // load libsm64
            sm64_global_init(romBuffer, mario_texture);
            sm64_audio_init(romBuffer);

            //for(int i=0; i<3*SM64_GEO_MAX_TRIANGLES; i++) mario_indices[i] = i;
            delete[] romBuffer;

            audio_thread_init();
            sm64_play_sound_global(SOUND_MENU_STAR_SOUND);

            loaded = true;
            message = "libsm64 loaded";
        }
    }

    static void tick()
    {
        if (!message.empty())
        {
            CHud::SetHelpMessage(message.c_str(), false, false, true);
            message.clear();
        }
    }

    sm64_san_andreas() {
        Events::initGameEvent.Add(init);
        Events::gameProcessEvent.Add(tick);
    }
} _sm64_san_andreas;
