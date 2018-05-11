#ifndef __MF_ENDIAN_H__
#define __MF_ENDIAN_H__

#include "mf_definitions.h"


static inline void mf_set64le(snap_membus_t & busline,
                              mfb_byteoffset_t lowest_byte,
                              snapu64_t value)
{
    snapu64_t pattern = 0;
    pattern( 7,  0) = value >>  0;
    pattern(15,  8) = value >>  8;
    pattern(23, 16) = value >> 16;
    pattern(31, 24) = value >> 24;
    pattern(39, 32) = value >> 32;
    pattern(47, 40) = value >> 40;
    pattern(55, 48) = value >> 48;
    pattern(63, 56) = value >> 56;

    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit+63, o_bit) = pattern;
}

static inline void mf_set64be(snap_membus_t & busline,
                              mfb_byteoffset_t lowest_byte,
                              snapu64_t value)
{
    snapu64_t pattern = 0;
    pattern( 7,  0) = value >> 56;
    pattern(15,  8) = value >> 48;
    pattern(23, 16) = value >> 40;
    pattern(31, 24) = value >> 32;
    pattern(39, 32) = value >> 24;
    pattern(47, 40) = value >> 16;
    pattern(55, 48) = value >>  8;
    pattern(63, 56) = value >>  0;

    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit+63, o_bit) = pattern;
}

static inline snapu64_t mf_get64le(const snap_membus_t & busline,
                                   mfb_byteoffset_t lowest_byte)
{
    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    snapu64_t pattern = busline(o_bit+63, o_bit);

    return ((snapu64_t)pattern( 7,  0) <<  0) +
           ((snapu64_t)pattern(15,  8) <<  8) +
           ((snapu64_t)pattern(23, 16) << 16) +
           ((snapu64_t)pattern(31, 24) << 24) +
           ((snapu64_t)pattern(39, 32) << 32) +
           ((snapu64_t)pattern(47, 40) << 40) +
           ((snapu64_t)pattern(55, 48) << 48) +
           ((snapu64_t)pattern(63, 56) << 56);
}

static inline snapu64_t mf_get64be(const snap_membus_t & busline,
                                   mfb_byteoffset_t lowest_byte)
{
    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    snapu64_t pattern = busline(o_bit+63, o_bit);

    return ((snapu64_t)pattern( 7,  0) << 56) +
           ((snapu64_t)pattern(15,  8) << 48) +
           ((snapu64_t)pattern(23, 16) << 40) +
           ((snapu64_t)pattern(31, 24) << 32) +
           ((snapu64_t)pattern(39, 32) << 24) +
           ((snapu64_t)pattern(47, 40) << 16) +
           ((snapu64_t)pattern(55, 48) <<  8) +
           ((snapu64_t)pattern(63, 56) <<  0);
}

template<int lowest_byte>
static snapu64_t mf_get64be(const snap_membus_t & busline)
{
    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    snapu64_t pattern = busline(o_bit+63, o_bit);

    return ((snapu64_t)pattern( 7,  0) << 56) +
           ((snapu64_t)pattern(15,  8) << 48) +
           ((snapu64_t)pattern(23, 16) << 40) +
           ((snapu64_t)pattern(31, 24) << 32) +
           ((snapu64_t)pattern(39, 32) << 24) +
           ((snapu64_t)pattern(47, 40) << 16) +
           ((snapu64_t)pattern(55, 48) <<  8) +
           ((snapu64_t)pattern(63, 56) <<  0);
}


static inline void mf_set32le(snap_membus_t & busline,
                              mfb_byteoffset_t lowest_byte,
                              snapu32_t value)
{
    snapu32_t pattern = 0;
    pattern( 7,  0) = value >>  0;
    pattern(15,  8) = value >>  8;
    pattern(23, 16) = value >> 16;
    pattern(31, 24) = value >> 24;

    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit+31, o_bit) = pattern;
}

static inline void mf_set32be(snap_membus_t & busline,
                              mfb_byteoffset_t lowest_byte,
                              snapu32_t value)
{
    snapu32_t pattern = 0;
    pattern( 7,  0) = value >> 24;
    pattern(15,  8) = value >> 16;
    pattern(23, 16) = value >>  8;
    pattern(31, 24) = value >>  0;

    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit+31, o_bit) = pattern;
}

static inline snapu32_t mf_get32le(const snap_membus_t & busline,
                                   mfb_byteoffset_t lowest_byte)
{
    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    snapu32_t pattern = busline(o_bit+31, o_bit);

    return ((snapu32_t)pattern( 7,  0) <<  0) +
           ((snapu32_t)pattern(15,  8) <<  8) +
           ((snapu32_t)pattern(23, 16) << 16) +
           ((snapu32_t)pattern(31, 24) << 24);
}

static inline snapu32_t mf_get32be(const snap_membus_t & busline,
                                   mfb_byteoffset_t lowest_byte)
{
    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    snapu32_t pattern = busline(o_bit+31, o_bit);

    return ((snapu32_t)pattern( 7,  0) << 24) +
           ((snapu32_t)pattern(15,  8) << 16) +
           ((snapu32_t)pattern(23, 16) <<  8) +
           ((snapu32_t)pattern(31, 24) <<  0);
}


static inline void mf_set16le(snap_membus_t & busline,
                              mfb_byteoffset_t lowest_byte,
                              snapu16_t value)
{
    snapu16_t pattern = 0;
    pattern( 7,  0) = value >> 0;
    pattern(15,  8) = value >> 8;

    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit+15, o_bit) = pattern;
}

static inline void mf_set16be(snap_membus_t & busline,
                              mfb_byteoffset_t lowest_byte,
                              snapu16_t value)
{
    snapu16_t pattern = 0;
    pattern( 7,  0) = value >> 8;
    pattern(15,  8) = value >> 0;

    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit+15, o_bit) = pattern;
}

static inline snapu16_t mf_get16le(const snap_membus_t & busline,
                                   mfb_byteoffset_t lowest_byte)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    snapu16_t pattern = busline(o_bit+15, o_bit);

    return ((snapu16_t)pattern( 7,  0) << 0) +
           ((snapu16_t)pattern(15,  8) << 8);
}

static inline snapu16_t mf_get16be(const snap_membus_t & busline,
                                   mfb_byteoffset_t lowest_byte)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    snapu16_t pattern = busline(o_bit+15, o_bit);

    return ((snapu16_t)pattern( 7,  0) << 8) +
           ((snapu16_t)pattern(15,  8) << 0);
}


static inline void mf_set8(snap_membus_t & busline,
                           mfb_byteoffset_t lowest_byte,
                           snapu8_t value)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit +  7, o_bit +  0) = value;
}

static inline snapu8_t mf_get8(const snap_membus_t & busline,
                               mfb_byteoffset_t lowest_byte)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    return busline(o_bit +  7, o_bit +  0);
}


#define mf_get64 mf_get64be
#define mf_set64 mf_set64be
#define mf_get32 mf_get32be
#define mf_set32 mf_set32be
#define mf_get16 mf_get16be
#define mf_set16 mf_set16be

//#define mf_get64 mf_get64le
//#define mf_set64 mf_set64le
//#define mf_get32 mf_get32le
//#define mf_set32 mf_set32le
//#define mf_get16 mf_get16le
//#define mf_set16 mf_set16le

#endif // __MF_ENDIAN_H__
