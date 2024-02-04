#include "plugin.h"
#include "CCamera.h"
#include "CHud.h"
#include "CMenuSystem.h"
#include "CTimer.h"
#include "CTimeCycle.h"
#include "CShadows.h"
#include "Fx_c.h"

#include "TinySHA1.hpp"
extern "C" {
    #include <decomp/include/PR/ultratypes.h>
    #include <decomp/include/audio_defines.h>
}

#include "config.h"
#include "audio.h"
#include "d3d9_funcs.h"
#include "mario.h"
#include "mario_render.h"
#include "mario_custom_anims.h"

using namespace plugin;

template<typename Type1, size_t N1, typename Type2, size_t N2, typename Type3, size_t N3, typename Type4, size_t N4, typename Type5, size_t N5, typename Type6, size_t N6, typename Type7, size_t N7>
using ArgPick7N = ArgPick<ArgTypes<Type1, Type2, Type3, Type4, Type5, Type6, Type7>, N1, N2, N3, N4, N5, N6, N7>;

bool loaded;
uint8_t* marioTexture;
RwImVertexIndex marioIndices[SM64_GEO_MAX_TRIANGLES * 3];

CdeclEvent<AddressList<0x53eac4, H_CALL,
                       0x705322, H_CALL,
                       0x7271e3, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> pedRenderWeaponsEvent;
CdeclEvent<AddressList<0x5e6900, H_CALL>, PRIORITY_AFTER, ArgPick7N<CPed*, 0, float, 1, float, 2, float, 3, float, 4, float, 5, float, 6>, void(CPed*, float, float, float, float, float, float)> pedStoreShadowsEvent;
ThiscallEvent<AddressList<0x5e8a29, H_JUMP,
                          0x6d3dc7, H_CALL,
                          0x6d3de6, H_CALL>, PRIORITY_BEFORE, ArgPickN<CPed*, 0>, void(CPed*)> pedPreRenderAfterTestEvent;

uint16_t CalculateShadowStrength(float currDist, float maxDist, uint16_t maxStrength) {
    //assert(maxDist >= currDist); // Otherwise integer underflow will occur
    if (maxDist >= currDist)
		return maxStrength;

    const auto halfMaxDist = maxDist / 2.f;
    if (currDist >= halfMaxDist) { // Anything further than half the distance is faded out
        return (uint16_t)((1.f - (currDist - halfMaxDist) / halfMaxDist) * maxStrength);
    } else { // Anything closer than half the max distance is full strength
        return (uint16_t)maxStrength;
    }
}

class sm64_san_andreas {
public:
    static void init()
    {
        if (loaded) return;

        loadConfig();

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

        if (!config["skip_sha1_checksum"])
        {
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

        if (config["autospawn_mario_on_start"]) marioSpawn();
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
            /*
            else if (KeyPressed(VK_NUMPAD9))
            {
                keyPressTime = CTimer::m_snTimeInMilliseconds - (KeyPressed(VK_NUMPAD0) ? 800 : 975);
                if (triangles < SM64_GEO_MAX_TRIANGLES) triangles++;
                char msg[32];
                sprintf(msg, "%d", triangles);
                CHud::SetHelpMessage(msg, 0,0,0);
            }
            else if (KeyPressed(VK_NUMPAD6))
            {
                keyPressTime = CTimer::m_snTimeInMilliseconds - (KeyPressed(VK_NUMPAD0) ? 800 : 975);
                if (triangles > 0) triangles--;
                char msg[32];
                sprintf(msg, "%d", triangles);
                CHud::SetHelpMessage(msg, 0,0,0);
            }
            */
        }

        marioTick((CTimer::m_snTimeInMilliseconds - CTimer::m_snPreviousTimeInMilliseconds) / 1000.f);
    }

    static void marioSetupShadow(CPed* ped, float displacementX, float displacementY, float frontX, float frontY, float sideX, float sideY)
    {
        if (!marioSpawned() || !ped->IsPlayer()) return;

        if (CShadows::ShadowsStoredToBeRendered)
            CShadows::ShadowsStoredToBeRendered--; // this makes it so that CJ's shadow index is replaced with an entry for Mario

        const auto& camPos = TheCamera.GetPosition();
        const auto pedToCamDist2DSq = (marioInterpPos - camPos).Magnitude2D();
        const auto strength = (uint8_t)CalculateShadowStrength(std::sqrt(pedToCamDist2DSq), MAX_DISTANCE_PED_SHADOWS, CTimeCycle::m_CurrentColours.m_nShadowStrength);
        const auto pos = marioInterpPos + CVector{displacementX, displacementY, 0.15f};
        CShadows::StoreShadowToBeRendered(
            (uint8_t)SHADOW_DEFAULT,
            gpShadowPedTex,
            &pos,
            frontX, frontY,
            sideX, sideY,
            (int16_t)strength,
            strength, strength, strength,
            4.f,
            false,
            1.f,
            nullptr,
            g_fx.GetFxQuality() >= FXQUALITY_VERY_HIGH
        );
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

        pedRenderWeaponsEvent.before.Add(marioRenderWeapon);
        pedStoreShadowsEvent.after.Add(marioSetupShadow);
        pedPreRenderAfterTestEvent.before.Add(marioPreRender);
        pedPreRenderAfterTestEvent.after.Add(marioPreRenderReset);
    }
} _sm64_san_andreas;
