#include "mtl_op_file.h"

#include "mtl_file.h"

#include <snap_types.h>


mtl_extmap_t read_extmap;
mtl_extmap_t write_extmap;

// #define READ_SLOT 0
// #define WRITE_SLOT 1

// static uint64_t _read_offset = 0;
// static uint64_t _read_size = 0;

// static uint64_t _write_offset = 0;
// static uint64_t _write_size = 0;

// mtl_retc_t op_file_set_read_buffer(uint64_t read_offset, uint64_t read_size) {
//     _read_offset = read_offset;
//     _read_size = read_size;
//     return SNAP_RETC_SUCCESS;
// }
// mtl_retc_t op_file_set_write_buffer(uint64_t write_offset, uint64_t write_size) {
//     _write_offset = write_offset;
//     _write_size = write_size;
//     return SNAP_RETC_SUCCESS;
// }

// void op_file_read(snap_membus_t *mem_ddr, mtl_stream &out, snap_bool_t enable) {
//     const snapu64_t file_blocks = mtl_file_get_block_count(READ_SLOT);
//     const snapu64_t file_bytes = file_blocks * MTL_BLOCK_BYTES;
//     if (_read_offset + _read_size > file_bytes)
//     {
//         return ;//SNAP_RETC_FAILURE;
//     }

//     const mtl_block_offset_t file_begin_offset = MTL_BLOCK_OFFSET(_read_offset);
//     const snapu64_t file_begin_block = MTL_BLOCK_NUMBER(_read_offset);
//     const mtl_block_offset_t file_end_offset = MTL_BLOCK_OFFSET(_read_offset + _read_size - 1);
//     const snapu64_t file_end_block = MTL_BLOCK_NUMBER(_read_offset + _read_size - 1);

//     snapu64_t word = 0;
//     ap_uint<4> bytes_read = 0;
//     if (! mtl_file_seek(mem_ddr, READ_SLOT, file_begin_block, MTL_FALSE)) return ;//SNAP_RETC_FAILURE;
//     for (snapu64_t i_block = file_begin_block; i_block <= file_end_block; ++i_block) {
//         mtl_block_offset_t begin_offset = 0; if (i_block == file_begin_block)
//         {
//             begin_offset = file_begin_offset;
//         }
//         mtl_block_offset_t end_offset = MTL_BLOCK_BYTES - 1;
//         if (i_block == file_end_block) {
//             end_offset = file_end_offset;
//         }

//         for (mtl_block_count_t i_byte = begin_offset; i_byte <= end_offset; ++i_byte) {
//             word = (word<<8) | mtl_file_buffers[READ_SLOT][i_byte];
//             ++bytes_read;
//             if (bytes_read == 8)
//             {
//                 bytes_read = 0;
//                 mtl_bool_t last = (i_byte == end_offset) && (i_block == file_end_block);
//                 mtl_stream_element element = {word, 0xff, last};
//                 out.write(element);
//             }
//         }

//         if (i_block == file_end_block) {
//             if (! mtl_file_flush(mem_ddr, READ_SLOT)) return ;//SNAP_RETC_FAILURE;
//         } else {
//             if (! mtl_file_next(mem_ddr, READ_SLOT, MTL_FALSE)) return ;//SNAP_RETC_FAILURE;
//         }
//     }
//     return ;//SNAP_RETC_SUCCESS;
// }

// void op_file_write(mtl_stream & in, snap_membus_t *mem_ddr, snap_bool_t enable) {
//     const snapu64_t file_blocks = mtl_file_get_block_count(WRITE_SLOT);
//     const snapu64_t file_bytes = file_blocks * MTL_BLOCK_BYTES;
//     if (_read_offset + _read_size > file_bytes)
//     {
//         return ;//SNAP_RETC_FAILURE;
//     }

//     const mtl_block_offset_t file_begin_offset = MTL_BLOCK_OFFSET(_read_offset);
//     const snapu64_t file_begin_block = MTL_BLOCK_NUMBER(_read_offset);
//     const mtl_block_offset_t file_end_offset = MTL_BLOCK_OFFSET(_read_offset + _read_size - 1);
//     const snapu64_t file_end_block = MTL_BLOCK_NUMBER(_read_offset + _read_size - 1);

//     snapu64_t i_block = file_begin_block;
//     mtl_block_offset_t i_byte = file_begin_offset;
//     if (! mtl_file_seek(mem_ddr, WRITE_SLOT, i_block, MTL_FALSE)) return ;//SNAP_RETC_FAILURE;
//     mtl_block_offset_t end_offset = MTL_BLOCK_BYTES - 1;
//     if (i_block == file_end_block) {
//         end_offset = file_end_offset;
//     }

//     snapu64_t word = 0;
//     mtl_stream_element element;
//     do {
//         in.read(element);
//         word = element.data;

//         for (ap_uint<4> i_byte_in_word = 0; i_byte_in_word < 8; ++i_byte_in_word)
//         {
//             if ((element.strb << i_byte_in_word) & 0x80) {
//                 mtl_file_buffers[WRITE_SLOT ][i_byte] = (word >> (56 - i_byte_in_word*8)) & 0xff;

//                 if (i_byte == end_offset) {
//                     if (i_block == file_end_block) {
//                         if (! mtl_file_flush(mem_ddr, WRITE_SLOT)) return ;//SNAP_RETC_FAILURE;
//                         return ;//element.last ? SNAP_RETC_SUCCESS : SNAP_RETC_FAILURE;
//                     } else {
//                         if (! mtl_file_next(mem_ddr, WRITE_SLOT, MTL_TRUE)) return ;//SNAP_RETC_FAILURE;
//                         ++i_block;
//                         if (i_block == file_end_block) {
//                             end_offset = file_end_offset;
//                         }
//                     }
//                 } else {
//                     ++i_byte;
//                 }
//             }
//         }
//     } while(!element.last);

//     return ;//SNAP_RETC_SUCCESS;
// }
