#pragma once

static inline snapu64_t mf_get64le(const snap_membus_t & busline,
                                    mf_busline_offset_t lowest_byte)
{
    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    return (snapu64_t)busline(o_bit +  7, o_bit +  0) <<  0 +
           (snapu64_t)busline(o_bit + 15, o_bit +  8) <<  8 +
           (snapu64_t)busline(o_bit + 23, o_bit + 16) << 16 +
           (snapu64_t)busline(o_bit + 31, o_bit + 24) << 24 +
           (snapu64_t)busline(o_bit + 39, o_bit + 32) << 32 +
           (snapu64_t)busline(o_bit + 47, o_bit + 40) << 40 +
           (snapu64_t)busline(o_bit + 55, o_bit + 48) << 48 +
           (snapu64_t)busline(o_bit + 63, o_bit + 56) << 56;
}

static inline void mf_set64le(snap_membus_t & busline,
                            mf_busline_offset_t lowest_byte,
                            snapu64_t value)
{
    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit +  7, o_bit +  0) = value >>  0;
    busline(o_bit + 15, o_bit +  8) = value >>  8;
    busline(o_bit + 23, o_bit + 16) = value >> 16;
    busline(o_bit + 31, o_bit + 24) = value >> 24;
    busline(o_bit + 39, o_bit + 32) = value >> 32;
    busline(o_bit + 47, o_bit + 40) = value >> 40;
    busline(o_bit + 55, o_bit + 48) = value >> 48;
    busline(o_bit + 63, o_bit + 56) = value >> 56;
}

static inline snapu64_t mf_get64be(const snap_membus_t & busline,
                                    mf_busline_offset_t lowest_byte)
{
    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    return (snapu64_t)busline(o_bit +  7, o_bit +  0) << 56 +
           (snapu64_t)busline(o_bit + 15, o_bit +  8) << 48 +
           (snapu64_t)busline(o_bit + 23, o_bit + 16) << 40 +
           (snapu64_t)busline(o_bit + 31, o_bit + 24) << 32 +
           (snapu64_t)busline(o_bit + 39, o_bit + 32) << 24 +
           (snapu64_t)busline(o_bit + 47, o_bit + 40) << 16 +
           (snapu64_t)busline(o_bit + 55, o_bit + 48) <<  8 +
           (snapu64_t)busline(o_bit + 63, o_bit + 56) <<  0;
}

static inline void mf_set64be(snap_membus_t & busline,
                                mf_busline_offset_t lowest_byte,
                                snapu64_t value)
{
    mfb_bitoffset_t o_bit = MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit +  7, o_bit +  0) = value << 56;
    busline(o_bit + 15, o_bit +  8) = value << 48;
    busline(o_bit + 23, o_bit + 16) = value << 40;
    busline(o_bit + 31, o_bit + 24) = value << 32;
    busline(o_bit + 39, o_bit + 32) = value << 24;
    busline(o_bit + 47, o_bit + 40) = value << 16;
    busline(o_bit + 55, o_bit + 48) = value <<  8;
    busline(o_bit + 63, o_bit + 56) = value <<  0;
}

static inline snapu32_t mf_get32le(const snap_membus_t & busline, mf_busline_offset_t lowest_byte)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    return (snapu32_t)busline(o_bit +  7, o_bit +  0) <<  0 +
           (snapu32_t)busline(o_bit + 15, o_bit +  8) <<  8 +
           (snapu32_t)busline(o_bit + 23, o_bit + 16) << 16 +
           (snapu32_t)busline(o_bit + 31, o_bit + 24) << 24;
}

static inline void mf_set32le(snap_membus_t & busline, mf_busline_offset_t lowest_byte, snapu32_t value)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit +  7, o_bit +  0) = value >>  0;
    busline(o_bit + 15, o_bit +  8) = value >>  8;
    busline(o_bit + 23, o_bit + 16) = value >> 16;
    busline(o_bit + 31, o_bit + 24) = value >> 24;
}

static inline snapu32_t mf_get32be(const snap_membus_t & busline, mf_busline_offset_t lowest_byte)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    return (snapu32_t)busline(o_bit +  7, o_bit +  0) << 24 +
           (snapu32_t)busline(o_bit + 15, o_bit +  8) << 16 +
           (snapu32_t)busline(o_bit + 23, o_bit + 16) <<  8 +
           (snapu32_t)busline(o_bit + 31, o_bit + 24) <<  0;
}

static inline void mf_set32be(snap_membus_t & busline, mf_busline_offset_t lowest_byte, snapu32_t value)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit +  7, o_bit +  0) = value >> 24;
    busline(o_bit + 15, o_bit +  8) = value >> 16;
    busline(o_bit + 23, o_bit + 16) = value >>  8;
    busline(o_bit + 31, o_bit + 24) = value >>  0;
}

static inline snapu16_t mf_get16le(const snap_membus_t & busline, mf_busline_offset_t lowest_byte)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    return (snapu16_t)busline(o_bit +  7, o_bit +  0) << 0 +
           (snapu16_t)busline(o_bit + 15, o_bit +  8) << 8;
}

static inline void mf_set16le(snap_membus_t & busline, mf_busline_offset_t lowest_byte, snapu16_t value)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit +  7, o_bit +  0) = value << 0;
    busline(o_bit + 15, o_bit +  8) = value << 8;
}

static inline snapu16_t mf_get16be(const snap_membus_t & busline, mf_busline_offset_t lowest_byte)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    return (snapu16_t)busline(o_bit +  7, o_bit +  0) << 8 +
           (snapu16_t)busline(o_bit + 15, o_bit +  8) << 0;
}

static inline void mf_set16be(snap_membus_t & busline, mf_busline_offset_t lowest_byte, snapu16_t value)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit +  7, o_bit +  0) = value << 8;
    busline(o_bit + 15, o_bit +  8) = value << 0;
}

static inline snapu8_t mf_get8(const snap_membus_t & busline, mf_busline_offset_t lowest_byte)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    return (snapu16_t)busline(o_bit +  7, o_bit +  0);
}

static inline void mf_set8(snap_membus_t & busline, mf_busline_offset_t lowest_byte, snapu8_t value)
{
    mfb_bitoffset_t o_bit= MFB_TOBITOFFSET(lowest_byte);
    busline(o_bit +  7, o_bit +  0) = value;
}

#define mf_get32 mf_get32be
#define mf_set32 mf_set32be
#define mf_get16 mf_get16be
#define mf_set16 mf_set16be
#define mf_get8  mf_get8be
#define mf_set8  mf_set8be

