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

template<int lowest_byte>
static inline snapu8_t mf_get8(const snap_membus_t & busline)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    return busline(o_bit +  7, o_bit +  0);
}

static inline snapu8_t mf_get8(const snap_membus_t & busline,
                               mfb_byteoffset_t lowest_byte)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    return busline(o_bit +  7, o_bit +  0);
}

static inline snap_membus_t swap_membus_endianness(snap_membus_t value) {
    return (
        mf_get8<0>(value),
        mf_get8<1>(value),
        mf_get8<2>(value),
        mf_get8<3>(value),
        mf_get8<4>(value),
        mf_get8<5>(value),
        mf_get8<6>(value),
        mf_get8<7>(value),
        mf_get8<8>(value),
        mf_get8<9>(value),
        mf_get8<10>(value),
        mf_get8<11>(value),
        mf_get8<12>(value),
        mf_get8<13>(value),
        mf_get8<14>(value),
        mf_get8<15>(value),
        mf_get8<16>(value),
        mf_get8<17>(value),
        mf_get8<18>(value),
        mf_get8<19>(value),
        mf_get8<20>(value),
        mf_get8<21>(value),
        mf_get8<22>(value),
        mf_get8<23>(value),
        mf_get8<24>(value),
        mf_get8<25>(value),
        mf_get8<26>(value),
        mf_get8<27>(value),
        mf_get8<28>(value),
        mf_get8<29>(value),
        mf_get8<30>(value),
        mf_get8<31>(value),
        mf_get8<32>(value),
        mf_get8<33>(value),
        mf_get8<34>(value),
        mf_get8<35>(value),
        mf_get8<36>(value),
        mf_get8<37>(value),
        mf_get8<38>(value),
        mf_get8<39>(value),
        mf_get8<40>(value),
        mf_get8<41>(value),
        mf_get8<42>(value),
        mf_get8<43>(value),
        mf_get8<44>(value),
        mf_get8<45>(value),
        mf_get8<46>(value),
        mf_get8<47>(value),
        mf_get8<48>(value),
        mf_get8<49>(value),
        mf_get8<50>(value),
        mf_get8<51>(value),
        mf_get8<52>(value),
        mf_get8<53>(value),
        mf_get8<54>(value),
        mf_get8<55>(value),
        mf_get8<56>(value),
        mf_get8<57>(value),
        mf_get8<58>(value),
        mf_get8<59>(value),
        mf_get8<60>(value),
        mf_get8<61>(value),
        mf_get8<62>(value),
        mf_get8<63>(value)
    );
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
