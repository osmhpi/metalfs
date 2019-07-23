/********************************************
File name:      io_control
Author:         Jianyu Chen
School:         Delft Univsersity of Technology
Date:           10th Sept, 2018
Description:    The module to contol the input and output dataflow. 
                Each burst read will acquire 4K data, except the last burst read of a file (it can be less)
                Each burst write will write 4K data, also except the last one
                This module is to control the dataflow of axi protocal interface.
********************************************/
`timescale 1ns/1ps

module io_control(
    input clk,
    input rst_n,

    input[63:0] src_addr,
    output rd_req,
    input rd_req_ack,
    output[7:0] rd_len,
    output[63:0] rd_address,

    input wr_valid,
    input wr_ready,
    input[63:0] des_addr,
    output wr_req,
    input wr_req_ack,
    output[7:0] wr_len,
    output[63:0] wr_address,
    output bready,
    input bresp,

    input done_i,
    input start,
    output idle,
    output ready,
    output done_out,

    input[31:0] decompression_length,
    input[34:0] compression_length
);

/****************solved the read data*************/ 
reg[34:0] compression_length_r;   ///[34:12]:number of 4k blocks  [11:6]:number of 64B [5:0]:fraction
reg[63:0] rd_address_r;
reg[7:0] rd_len_r;
reg rd_req_r;
reg[2:0] rd_state;
reg read_done_r;

always@(posedge clk)begin
    if(~rst_n)begin
        rd_req_r        <= 1'b0;
        rd_state        <= 3'd0;
        read_done_r     <= 1'b0;
    end else case(rd_state)
        3'd0:begin
            if(start)begin
                //Round the length to the upper 64*n, n is an integer. Because the bandwidth is 64Byte
                if(compression_length[5:0]!=6'b0)begin
                    compression_length_r[34:6]  <= compression_length[34:6] + 29'd1;
                end else begin
                    compression_length_r[34:6]  <= compression_length[34:6];
                end

                rd_address_r            <= src_addr;
                rd_req_r                <= 1'b0;
                rd_state                <= 3'd1;
            end
        end
        3'd1:begin // the state to read the first 4KB chunk of the 64KB Snappy block
            //If the block is greater than 4KB (64*64), read a 4KB block. If not, read all the block
            if(compression_length_r[34:6]<=29'd64)begin
                rd_len_r                    <= {2'd0,compression_length_r[11:6]-6'd1};
                compression_length_r[34:6]  <= 29'd0;
                rd_state                    <= 3'd3;
            end else begin
                rd_len_r                    <= 8'b11_1111;
                compression_length_r[34:6]    <= compression_length_r[34:6]-29'd64;
                rd_state                    <= 3'd2;
            end
            rd_req_r                <= 1'b1;
        end
        3'd2:begin//the state to read the the block
            //once get an acknowlege, read the next chunk
            if(rd_req_ack)begin
                rd_address_r                    <= rd_address_r+64'd4096;
                if(compression_length_r[34:6]<=29'd64)begin
                    rd_state                    <= 3'd3;
                    rd_len_r                    <= {2'd0,compression_length_r[11:6]-6'd1};
                    compression_length_r[34:6]  <= 29'd0;
                end else begin
                    rd_len_r                    <= 8'b11_1111;
                    compression_length_r[34:6]    <= compression_length_r[34:6]-29'd64;
                end
            end
        end
        3'd3:begin//state to reset the rd_req_ack
            if(rd_req_ack)begin
                rd_req_r                <= 1'b0;
                rd_state                <= 3'd4;
            end
        end
        3'd4:begin
            read_done_r        <= 1'b1;
            rd_state        <= 3'd0;
        end

        default:rd_state    <= 3'd0;
    endcase
end

/****************write data*****************/
reg[31:0] decompression_length_r;   ///[32:12]:number of 4k blocks  [11:6]:number of 64B [5:0]:fraction
reg[63:0] wr_address_r;
reg[2:0] wr_state;
reg[7:0] wr_len_r;
reg wr_req_r;
reg[63:0] wr_req_count;
reg[63:0] wr_done_count;    // a counter to count the write_done of the data write before the done signal is sent.
reg done_out_r;
always@(posedge clk)begin
    if(~rst_n)begin
        wr_state        <= 3'd0;
        wr_req_r        <= 1'b0;
        wr_req_count    <= 64'b0;
        done_out_r        <= 1'b0;
    end else case(wr_state)
        3'd0:begin                // initial state
            if(start)begin
                //similar to the read case
                if(decompression_length[5:0]!=6'b0)begin
                    decompression_length_r[31:6]    <= decompression_length[31:6]+29'd1;
                end else begin
                    decompression_length_r[31:6]    <= decompression_length[31:6];
                end
				
				wr_req_count    <= 64'b0;
                wr_state        <= 3'd1;
                wr_req_r        <= 1'b0;
                wr_address_r    <= des_addr;
            end
        end
        3'd1:begin                // state for sending the first 4K block
            if(decompression_length_r[31:6]<=26'd64)begin
                wr_len_r                        <= {2'b0,decompression_length_r[11:6]-6'd1};
                decompression_length_r[31:6]    <= 26'd0;
                wr_state                        <= 3'd3;
            end else begin
                wr_len_r                        <= 8'b11_1111;
                decompression_length_r[31:6]    <= decompression_length_r[31:6]-26'd64;
                wr_state                        <= 3'd2;
            end
            wr_req_r    <= 1'b1;
        end
        3'd2:begin                // state for sending the rest 4K blocks
            if(wr_req_ack)begin
                wr_req_count    <= wr_req_count+64'b1;
                wr_address_r    <= wr_address_r+64'd4096;
                if(decompression_length_r[31:6]<=26'd64)begin
                    wr_len_r                        <= {2'b0,decompression_length_r[11:6]-6'd1};
                    decompression_length_r[31:6]    <= 26'd0;
                    wr_state                        <= 3'd3;
                end else begin
                    wr_len_r                        <= 8'b11_1111;
                    decompression_length_r[31:6]    <= decompression_length_r[31:6]-26'd64;
                end
            end
        end
        3'd3:begin                // state for waiting the last wr_req_r is acknolodged
            if(wr_req_ack)begin
                wr_req_count    <= wr_req_count+64'b1;
                wr_req_r        <= 1'b0;
                wr_state        <= 3'd4;
            end
        end

        3'd4:begin
            if(wr_done_count==wr_req_count && read_done_r) begin    //write request ack count equal to write data ack count and the read is done
                done_out_r  <= 1'b1;
                wr_state    <= 3'd0;
            end
        end

        default:wr_state    <= 3'd0;
    endcase
end

always@(posedge clk)
begin
    if(~rst_n)
    begin
        wr_done_count    <=64'b0;
    end
    else if(start)
    begin
        wr_done_count    <=64'b0;
    end
    else if(bresp)
    begin
        wr_done_count    <=wr_done_count+64'b1;
    end
end

reg idle_r;
reg bready_r;
reg ready_r;
always@(posedge clk)begin
    if(~rst_n)begin
        idle_r      <= 1'b1;
        bready_r    <= 1'b0;
    end else if(start)begin
        idle_r      <= 1'b0;
        bready_r    <= 1'b1;
    end else if(done_i && done_out_r)begin
        idle_r      <= 1'b1;
        bready_r    <= 1'b0;
    end
end

always@(posedge clk) begin
    if(~rst_n)begin
        ready_r    <= 1'b0;
    end else begin
        ready_r    <= 1'b1;
    end
end

assign rd_address   = rd_address_r;
assign rd_req       = rd_req_r;
assign rd_len       = rd_len_r;
assign idle         = idle_r;
assign ready        = ready_r;

assign wr_address   = wr_address_r;
assign wr_req       = wr_req_r;
assign wr_len       = wr_len_r;
assign bready       = bready_r;

assign done_out     = done_out_r;

endmodule
