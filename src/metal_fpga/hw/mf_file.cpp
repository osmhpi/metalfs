#include "mf_file.h"

#include "mf_endian.h"


mf_bool_t mf_file_seek(mf_extmap_t & map,
                       mf_filebuf_t & buffer,
                       snap_membus_t * ddr,
                       snapu64_t lblock)
{
    if (!mf_extmap_seek(map, lblock)) {
        return MF_FALSE;
    }
    //TODO-lw READ BLOCK map.current_pblock
    return MF_TRUE;
}

mf_bool_t mf_file_next(mf_extmap_t & map,
                       mf_filebuf_t & buffer,
                       snap_membus_t * ddr)
{
    if (!mf_extmap_next(map)) {
        return MF_FALSE;
    }
    //TODO-lw READ BLOCK map.current_pblock
    return MF_TRUE;
}

void mf_file_flush(mf_extmap_t & slot,
                   mf_filebuf_t & buffer,
                   snap_membus_t * ddr)
{
    //TODO-lw WRITE BLOCK map.current_pblock
}

