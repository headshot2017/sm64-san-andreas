#include "load_anim_data.h"
#include "decomp/include/mario_animation_ids.h"

#include <stdlib.h>

static uint32_t s_num_entries = 0;
static struct Animation *s_libsm64_mario_animations = NULL;

#define ANIM_DATA_ADDRESS 0x004EC000

static uint16_t read_u16_be( const uint8_t *p )
{
    return
        (uint32_t)p[0] << 8 |
        (uint32_t)p[1];

}

static uint16_t read_s16_be( const uint8_t *p )
{
    return (int16_t)read_u16_be( p );
}

static uint32_t read_u32_be( const uint8_t *p )
{
    return
        (uint32_t)p[0] << 24 |
        (uint32_t)p[1] << 16 |
        (uint32_t)p[2] <<  8 |
        (uint32_t)p[3];
}

static void read_anim_data(const uint8_t* read_ptr, const uint32_t size, struct Animation* anim)
{
    const uint8_t *initial_ptr = read_ptr;

    anim->flags             = read_s16_be( initial_ptr ); initial_ptr += 2;
    anim->animYTransDivisor = read_s16_be( initial_ptr ); initial_ptr += 2;
    anim->startFrame        = read_s16_be( initial_ptr ); initial_ptr += 2;
    anim->loopStart         = read_s16_be( initial_ptr ); initial_ptr += 2;
    anim->loopEnd           = read_s16_be( initial_ptr ); initial_ptr += 2;
    anim->unusedBoneCount   = read_s16_be( initial_ptr ); initial_ptr += 2;
    uint32_t values_offset = read_u32_be( initial_ptr ); initial_ptr += 4;
    uint32_t index_offset  = read_u32_be( initial_ptr ); initial_ptr += 4;
    uint32_t end_offset    = read_u32_be( initial_ptr );
    if (!end_offset)
        end_offset = size;

    const uint8_t *index_ptr  = read_ptr + index_offset;
    const uint8_t *values_ptr = read_ptr + values_offset;
    const uint8_t *end_ptr    = read_ptr + end_offset;

    anim->index = malloc( values_offset - index_offset );
    anim->values = malloc( end_offset - values_offset );

    int j = 0;
    while( index_ptr < values_ptr )
    {
        anim->index[j++] = read_u16_be( index_ptr );
        index_ptr += 2;
    }

    j = 0;
    while( index_ptr < end_ptr )
    {
        anim->values[j++] = read_u16_be( index_ptr );
        index_ptr += 2;
    }
}

void load_mario_anims_from_rom( const uint8_t *rom )
{
    #define GET_OFFSET( n ) (read_u32_be((uint8_t*)&((struct OffsetSizePair*)( rom + ANIM_DATA_ADDRESS + 8 + (n)*8 ))->offset))
    #define GET_SIZE(   n ) (read_u32_be((uint8_t*)&((struct OffsetSizePair*)( rom + ANIM_DATA_ADDRESS + 8 + (n)*8 ))->size  ))

    const uint8_t *read_ptr = rom + ANIM_DATA_ADDRESS;
    s_num_entries = read_u32_be( read_ptr );

    s_libsm64_mario_animations = malloc( s_num_entries * sizeof( struct Animation ));
    struct Animation *anims = s_libsm64_mario_animations;

    for( int i = 0; i < s_num_entries; ++i )
    {
        read_ptr = rom + ANIM_DATA_ADDRESS + GET_OFFSET(i);

        read_anim_data(read_ptr, 0, &anims[i]);
    }

    #undef GET_OFFSET
    #undef GET_SIZE
}

uint32_t load_mario_custom_anim_from_data( const uint8_t *data, const uint32_t size )
{
    s_num_entries++;
    s_libsm64_mario_animations = realloc(s_libsm64_mario_animations, s_num_entries*sizeof(struct Animation));
    struct Animation *anim = s_libsm64_mario_animations + (s_num_entries-1);

    read_anim_data(data+24, size, anim);

    return s_num_entries-1;
}

void load_mario_animation(struct MarioAnimation *a, u32 index)
{
    if (a->currentAnimAddr != 1 + index) {
        a->currentAnimAddr = 1 + index;
        a->targetAnim = &s_libsm64_mario_animations[index];
    }
}

void unload_mario_anims( void )
{
    for( int i = 0; i < s_num_entries; ++i )
    {
        free( s_libsm64_mario_animations[i].index );
        free( s_libsm64_mario_animations[i].values );
    }

    free( s_libsm64_mario_animations );
    s_libsm64_mario_animations = NULL;
    s_num_entries = 0;
}
