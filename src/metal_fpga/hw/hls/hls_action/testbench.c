#ifndef __SYNTHESIS__

#include <stdio.h>
#include <endian.h>

int test_op_mem_read() {
    static snap_membus_t src_mem[16];
    static uint8_t *src_memb = (uint8_t *) src_mem;
    static mtl_stream out_stream;

    // Fill the memory with data
    for (int i = 0; i < 1024; ++i) {
        src_memb[i] = i % 256;
    }

    bool success = true;
    uint64_t offset, size;

    {
        // Op mem read should stream entire memory line
        offset = 0, size = 64;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mtl_stream_element result;
        for (int element = 0; element < 8; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream two entire memory lines
        offset = 0, size = 128;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mtl_stream_element result;
        for (int element = 0; element < 2 * 8; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream a half memory line starting at offset
        offset = 16, size = 32;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mtl_stream_element result;
        for (int element = 0; element < 8 / 2; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8 + offset], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream one and a half memory line starting at offset
        offset = 16, size = 96;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        mtl_stream_element result;
        for (int element = 0; element < 12; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8 + offset], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream one and a half memory line
        offset = 0, size = 96;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mtl_stream_element result;
        for (int element = 0; element < 12; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8 + offset], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream a half stream word starting at offset
        offset = 16, size = 4;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        mtl_stream_element result = out_stream.read();
        success &= memcmp(&result.data, &src_memb[offset], 4) == 0;
        success &= result.strb == 0x0f;
        success &= result.last == true;

        if (!success) return -1;
    }

    return 0;
}

int test_op_mem_write() {
    static mtl_stream in_stream;
    static snap_membus_t dest_mem[16];
    static uint8_t *dest_memb = (uint8_t *) dest_mem;
    static mtl_stream_element stream_elements[128];
    static uint8_t reference_data[1024];

    // Fill the stream elements with data
    for (int i = 0; i < 1024; ++i) {
        reference_data[i] = i % 256;
    }
    for (int i = 0; i < 128; ++i) {
        uint8_t *data = (uint8_t*) &stream_elements[i].data;
        for (int j = 0; j < 8; ++j) {
            data[j] = (i * 8 + j) % 256;
        }
    }

    bool success = true;
    uint64_t offset, size;

    {
        // Op mem write should store entire memory line
        offset = 0, size = 64;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 8; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 7;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // Op mem write should store two entire memory lines
        offset = 0, size = 128;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 16; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 15;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // Op mem write should store a half memory line
        offset = 0, size = 32;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 4; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 3;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // Op mem write should store a half memory line at offset
        offset = 16, size = 32;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 4; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 3;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // Op mem write should store one and a half memory lines at offset
        offset = 16, size = 96;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 12; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 11;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    return 0;
}

int main()
{
    // Poor man's unit tests:
    if (test_op_mem_read()) printf("Test failure\n"); else printf("Test success\n");
    if (test_op_mem_write()) printf("Test failure\n"); else printf("Test success\n");

    static snap_membus_t host_gmem[1024];
    static snap_membus_t dram_gmem[1024];
    //static snapu32_t nvme_gmem[];
    action_reg act_reg;
    action_RO_config_reg act_config;

    // Streams
    static snapu32_t switch_ctrl[32];
    static mtl_stream stream_0_2;
    static mtl_stream stream_1_1; // Shortcut for disabled op
    static mtl_stream stream_2_3;
    static mtl_stream stream_3_4;
    static mtl_stream stream_4_5;
    static mtl_stream stream_5_6;
    static mtl_stream stream_6_7;
    static mtl_stream stream_7_0;

    mtl_stream &axis_s_0 = stream_7_0;
    mtl_stream &axis_s_1 = stream_1_1;
    mtl_stream &axis_s_2 = stream_0_2;
    mtl_stream &axis_s_3 = stream_2_3;
    mtl_stream &axis_s_4 = stream_3_4;
    mtl_stream &axis_s_5 = stream_4_5;
    mtl_stream &axis_s_6 = stream_5_6;
    mtl_stream &axis_s_7 = stream_6_7;

    mtl_stream &axis_m_0 = stream_0_2;
    mtl_stream &axis_m_1 = stream_1_1;
    mtl_stream &axis_m_2 = stream_2_3;
    mtl_stream &axis_m_3 = stream_3_4;
    mtl_stream &axis_m_4 = stream_4_5;
    mtl_stream &axis_m_5 = stream_5_6;
    mtl_stream &axis_m_6 = stream_6_7;
    mtl_stream &axis_m_7 = stream_7_0;

    // read action config:
    act_reg.Control.flags = 0x0;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    fprintf(stderr, "ACTION_TYPE:   %08x\nRELEASE_LEVEL: %08x\nRETC:          %04x\n",
        (unsigned int)act_config.action_type,
        (unsigned int)act_config.release_level,
        (unsigned int)act_reg.Control.Retc);


    // test action functions:

    fprintf(stderr, "// MAP slot 2 1000:7,2500:3,1700:8\n");
    uint64_t * job_mem = (uint64_t *)host_gmem;
    uint32_t * job_mem_h = (uint32_t *)host_gmem;
    uint8_t * job_mem_b = (uint8_t *)host_gmem;
    job_mem_b[0] = 2; // slot
    job_mem[1] = true; // map
    job_mem[2] = htobe64(3); // extent_count

    job_mem[8]  = htobe64(1000); // ext0.begin
    job_mem[9]  = htobe64(7);    // ext0.count
    job_mem[10] = htobe64(2500); // ext1.begin
    job_mem[11] = htobe64(3);    // ext1.count
    job_mem[12] = htobe64(1700); // ext2.begin
    job_mem[13] = htobe64(8);    // ext2.count

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_MAP;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// MAP slot 7 1200:8,1500:24\n");
    job_mem_b[0] = 7; // slot
    job_mem[1] = true; // map
    job_mem[2] = htobe64(2); // extent_count

    job_mem[8]  = htobe64(1200); // ext0.begin
    job_mem[9]  = htobe64(8);    // ext0.count
    job_mem[10] = htobe64(1500); // ext1.begin
    job_mem[11] = htobe64(24);   // ext1.count

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_MAP;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// QUERY slot 2, lblock 9\n");

    job_mem_b[0] = 2; // slot
    job_mem_b[1] = true; // query_mapping
    job_mem_b[2] = true; // query_state
    job_mem[1] = htobe64(9); // lblock

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_QUERY;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);


    fprintf(stderr, "// ACCESS slot 2, write 6K @ offset 3K\n");
    job_mem_b[0] = 2; // slot
    job_mem_b[1] = true; // write
    job_mem[1] = htobe64(128); // buffer beginning at mem[2]
    job_mem[2] = htobe64(3072); // 3K offset
    job_mem[3] = htobe64(6144); // 6K length

    snap_membus_t data_line = 0;
    mtl_set64(data_line, 0, 0xda7a111100000001ull);
    mtl_set64(data_line, 8, 0xda7a222200000002ull);
    mtl_set64(data_line, 16, 0xda7a333300000003ull);
    mtl_set64(data_line, 24, 0xda7a444400000004ull);
    mtl_set64(data_line, 32, 0xda7a555500000005ull);
    mtl_set64(data_line, 40, 0xda7a666600000006ull);
    mtl_set64(data_line, 48, 0xda7a777700000007ull);
    mtl_set64(data_line, 56, 0xda7a888800000008ull);
    for (int i = 0; i < 96; ++i) {
        host_gmem[2+i] = data_line;
    }

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_ACCESS;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// ACCESS slot 2, read 100 @ offset 3077\n");
    job_mem_b[0] = 2; // slot
    job_mem_b[1] = false; // read
    job_mem[1] = htobe64(7168); // buffer beginning at mem[112]
    job_mem[2] = htobe64(3077); // 3077 offset
    job_mem[3] = htobe64(100); // 100 length

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_ACCESS;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// CONFIGURE STREAMS\n");
    uint64_t enable = 0;
    enable |= (1 << 0);
    enable |= (1 << 1);
    // enable |= (1 << 2);
    // enable |= (1 << 3);
    enable |= (1 << 4);
    enable |= (1 << 5);
    enable |= (1 << 6);
    enable |= (1 << 7);
    enable |= (1 << 8);
    enable |= (1 << 9);

    job_mem[0] = htobe64(enable);

    uint32_t *master_source = job_mem_h + 2;
    master_source[0] = htobe32(7);
    master_source[1] = htobe32(0);
    master_source[2] = htobe32(1);
    master_source[3] = htobe32(2);
    master_source[4] = htobe32(3);
    master_source[5] = htobe32(4);
    master_source[6] = htobe32(5);
    master_source[7] = htobe32(6);

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_CONFIGURE_STREAMS;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    fprintf(stderr, "// OP MEM SET READ BUFFER\n");
    job_mem[0] = htobe64(0x80);
    job_mem[1] = htobe64(68);
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_OP_MEM_SET_READ_BUFFER;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    fprintf(stderr, "// OP MEM SET WRITE BUFFER\n");
    job_mem[0] = htobe64(0x100);
    job_mem[1] = htobe64(126);
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_OP_MEM_SET_WRITE_BUFFER;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    fprintf(stderr, "// OP CHANGE CASE SET MODE\n");
    job_mem[0] = htobe64(0);
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_OP_CHANGE_CASE_SET_MODE;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    fprintf(stderr, "// RUN OPERATORS\n");
    // Fill the memory with 'c' and 'd' characters
    memset(&job_mem_b[0x80], 0, 1024);
    memset(&job_mem_b[0x80], 'c', 64);
    memset(&job_mem_b[0x80 + 64], 'd', 64);

    // Dummy output data
    memset(&job_mem_b[0x100], 'e', 128);
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_RUN_OPERATORS;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    *(char *)(job_mem_b + 0x100 + 128) = '\0';
    fprintf(stderr, "Result is : %s\n", (char *)(job_mem_b + 0x100));

    fprintf(stderr, "// WRITEBACK \n");
        job_mem[0] = htobe64(0);
        job_mem[1] = htobe64(0);
        job_mem[2] = htobe64(0x80);
        job_mem[3] = htobe64(1);
        act_reg.Data.job_address = 0;
        act_reg.Data.job_type = MTL_JOB_WRITEBACK;
        hls_action(host_gmem, host_gmem,
    #ifdef NVME_ENABLED
            NULL,
    #endif
            axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);


    return 0;
}

#endif /* __SYNTHESIS__ */
