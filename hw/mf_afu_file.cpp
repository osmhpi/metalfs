#include "mf_afu_file.h"

#include "mf_file.h"

#include <snap_types.h>


#define READ_SLOT 0
#define WRITE_SLOT 1

static uint64_t _read_offset = 0;
static uint64_t _read_size = 0;

static uint64_t _write_offset = 0;
static uint64_t _write_size = 0;

mf_retc_t afu_file_set_read_buffer(uint64_t read_offset, uint64_t read_size) {
    _read_offset = read_offset;
    _read_size = read_size;
    return SNAP_RETC_SUCCESS;
}
mf_retc_t afu_file_set_write_buffer(uint64_t write_offset, uint64_t write_size) {
    _write_offset = write_offset;
    _write_size = write_size;
    return SNAP_RETC_SUCCESS;
}

void afu_file_read(snap_membus_t *mem_ddr, mf_stream &out, snap_bool_t enable) {
    const snapu64_t file_blocks = mf_file_get_block_count(READ_SLOT);
    const snapu64_t file_bytes = file_blocks * MF_BLOCK_BYTES;
    if (_read_offset + _read_size > file_bytes)
    {
        return ;//SNAP_RETC_FAILURE;
    }

    const mf_block_offset_t file_begin_offset = MF_BLOCK_OFFSET(_read_offset);
    const snapu64_t file_begin_block = MF_BLOCK_NUMBER(_read_offset);
    const mf_block_offset_t file_end_offset = MF_BLOCK_OFFSET(_read_offset + _read_size - 1);
    const snapu64_t file_end_block = MF_BLOCK_NUMBER(_read_offset + _read_size - 1);

    snapu64_t word = 0;
    ap_uint<4> bytes_read = 0;
    if (! mf_file_seek(mem_ddr, READ_SLOT, file_begin_block, MF_FALSE)) return ;//SNAP_RETC_FAILURE;
    for (snapu64_t i_block = file_begin_block; i_block <= file_end_block; ++i_block) {
        mf_block_offset_t begin_offset = 0; if (i_block == file_begin_block)
        {
            begin_offset = file_begin_offset;
        }
        mf_block_offset_t end_offset = MF_BLOCK_BYTES - 1;
        if (i_block == file_end_block) {
            end_offset = file_end_offset;
        }

        for (mf_block_count_t i_byte = begin_offset; i_byte <= end_offset; ++i_byte) {
            word = (word<<8) | mf_file_buffers[READ_SLOT][i_byte];
            ++bytes_read;
            if (bytes_read == 8)
            {
                bytes_read = 0;
                mf_bool_t last = (i_byte == end_offset) && (i_block == file_end_block);
                mf_stream_element element = {word, 0xff, last};
                out.write(element);
            }
        }

        if (i_block == file_end_block) {
            if (! mf_file_flush(mem_ddr, READ_SLOT)) return ;//SNAP_RETC_FAILURE;
        } else {
            if (! mf_file_next(mem_ddr, READ_SLOT, MF_FALSE)) return ;//SNAP_RETC_FAILURE;
        }
    }
    return ;//SNAP_RETC_SUCCESS;
}

void afu_file_write(mf_stream & in, snap_membus_t *mem_ddr, snap_bool_t enable) {
    const snapu64_t file_blocks = mf_file_get_block_count(WRITE_SLOT);
    const snapu64_t file_bytes = file_blocks * MF_BLOCK_BYTES;
    if (_read_offset + _read_size > file_bytes)
    {
        return ;//SNAP_RETC_FAILURE;
    }

    const mf_block_offset_t file_begin_offset = MF_BLOCK_OFFSET(_read_offset);
    const snapu64_t file_begin_block = MF_BLOCK_NUMBER(_read_offset);
    const mf_block_offset_t file_end_offset = MF_BLOCK_OFFSET(_read_offset + _read_size - 1);
    const snapu64_t file_end_block = MF_BLOCK_NUMBER(_read_offset + _read_size - 1);
    
    snapu64_t i_block = file_begin_block;
    mf_block_offset_t i_byte = file_begin_offset;
    if (! mf_file_seek(mem_ddr, WRITE_SLOT, i_block, MF_FALSE)) return ;//SNAP_RETC_FAILURE;
    mf_block_offset_t end_offset = MF_BLOCK_BYTES - 1;
    if (i_block == file_end_block) {
        end_offset = file_end_offset;
    }

    snapu64_t word = 0;
    mf_stream_element element;
    do {
        in.read(element);
        word = element.data;

        for (ap_uint<4> i_byte_in_word = 0; i_byte_in_word < 8; ++i_byte_in_word)
        {
            if ((element.strb << i_byte_in_word) & 0x80) {
                mf_file_buffers[WRITE_SLOT ][i_byte] = (word >> (56 - i_byte_in_word*8)) & 0xff;

                if (i_byte == end_offset) {
                    if (i_block == file_end_block) {
                        if (! mf_file_flush(mem_ddr, WRITE_SLOT)) return ;//SNAP_RETC_FAILURE;
                        return ;//element.last ? SNAP_RETC_SUCCESS : SNAP_RETC_FAILURE;
                    } else {
                        if (! mf_file_next(mem_ddr, WRITE_SLOT, MF_TRUE)) return ;//SNAP_RETC_FAILURE;
                        ++i_block;
                        if (i_block == file_end_block) {
                            end_offset = file_end_offset;
                        }
                    }
                } else {
                    ++i_byte;
                }
            }
        }
    } while(!element.last);

    return ;//SNAP_RETC_SUCCESS;
}
