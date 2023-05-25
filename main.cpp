#include "plugin.h"
#include "CHud.h"
#include "CMenuSystem.h"
#include "CTimer.h"

#include "TinySHA1.hpp"
extern "C" {
    #include <decomp/include/PR/ultratypes.h>
    #include <decomp/include/audio_defines.h>
}

#include "audio.h"
#include "d3d9_funcs.h"
#include "mario.h"
#include "mario_render.h"
#include "mario_custom_anims.h"

using namespace plugin;

bool loaded;
uint8_t* marioTexture;
RwImVertexIndex marioIndices[SM64_GEO_MAX_TRIANGLES * 3];

class sm64_san_andreas {
public:
    static void init()
    {
        if (loaded) return;

        std::ifstream file("sm64.us.z64", std::ios::ate | std::ios::binary);

        if (!file)
        {
            MessageBoxA(0, "Super Mario 64 US ROM not found!\nPlease provide a ROM with the filename \"sm64.us.z64\"", "sm64-san-andreas", 0);
            return;
        }

        // load ROM into memory
        uint8_t *romBuffer;
        size_t romFileLength = file.tellg();

        romBuffer = new uint8_t[romFileLength + 1];
        file.seekg(0);
        file.read((char*)romBuffer, romFileLength);
        romBuffer[romFileLength] = 0;

        // check ROM SHA1 to avoid crash
        sha1::SHA1 s;
        char hexdigest[256];
        uint32_t digest[5];
        s.processBytes(romBuffer, romFileLength);
        s.getDigest(digest);
        sprintf(hexdigest, "%08x", digest[0]);
        if (strcmp(hexdigest, "9bef1128"))
        {
            char msg[128];
            sprintf(msg, "Super Mario 64 US ROM checksum does not match!\nYou have the wrong ROM.\n\nExpected: 9bef1128\nYour copy: %s", hexdigest);
            MessageBoxA(0, msg, "sm64-san-andreas", 0);
            return;
        }

        // Mario texture is 704x64 RGBA
        marioTexture = new uint8_t[4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT];

        // load libsm64
        sm64_global_init(romBuffer, marioTexture);
        sm64_audio_init(romBuffer);
        sm64_set_sound_volume(0.5f);

        for(int i=0; i<3*SM64_GEO_MAX_TRIANGLES; i++) marioIndices[i] = i;
        delete[] romBuffer;

        for (int i=0; i<SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT; i++)
        {
            // swap red and blue colors
            uint8_t r = marioTexture[i*4+0];
            marioTexture[i*4+0] = marioTexture[i*4+2];
            marioTexture[i*4+2] = r;
        }

        sm64_register_wall_attack_function(onWallAttack);
        sm64_register_debug_print_function( [](const char* msg){printf("%s\n", msg);} );

        marioInitCustomAnims();

        audio_thread_init();
        sm64_play_sound_global(SOUND_MENU_STAR_SOUND);

        loaded = true;

        initD3D();
        marioRenderInit();
    }

    static void loadGame()
    {
        if (!loaded) return;
        if (marioSpawned()) marioDestroy();
    }

    static void destroy()
    {
        if (!loaded) return;

        marioRenderDestroy();
        marioDestroy();
        audio_thread_stop();
        sm64_global_terminate();
        destroyD3D();
        delete[] marioTexture;

        loaded = false;
    }

    static void tick()
    {
        if (CTimer::m_UserPause || !loaded) return;

        static int keyPressTime = 0;
        if (CTimer::m_snTimeInMilliseconds - keyPressTime > 1000)
        {
            if (KeyPressed('M'))
            {
                keyPressTime = CTimer::m_snTimeInMilliseconds;
                if (marioSpawned())
                    marioDestroy();
                else
                    marioSpawn();
            }
            else if (KeyPressed(VK_OEM_COMMA))
            {
                keyPressTime = CTimer::m_snTimeInMilliseconds;
                marioRenderToggleDebug();
            }
            else if (KeyPressed('P'))
            {
                keyPressTime = CTimer::m_snTimeInMilliseconds;
                marioTestAnim();
            }
        }

        marioTick((CTimer::m_snTimeInMilliseconds - CTimer::m_snPreviousTimeInMilliseconds) / 1000.f);
    }

    sm64_san_andreas() {
        loaded = false;
        marioTexture = nullptr;

        Events::initRwEvent.Add(init);
        Events::reInitGameEvent.Add(loadGame);
        Events::shutdownRwEvent.Add(destroy);

        Events::d3dLostEvent.Add(destroyD3D);
        Events::d3dResetEvent.Add(initD3D);

        Events::gameProcessEvent.Add(tick);
        Events::drawingEvent.Add(draw);

        Events::pedRenderEvent.before.Add(marioRenderPed);
        Events::pedRenderEvent.after.Add(marioRenderPedReset);
    }
} _sm64_san_andreas;
