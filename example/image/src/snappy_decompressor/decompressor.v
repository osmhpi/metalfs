/****************************
Module name: 	decompressor
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			24th Nov, 2018
Function:		The top level of the decompressor (without the axi protocal controller)
****************************/
`timescale 1ns/1ps

module decompressor(
	input clk,
	input rst_n,
	input[511:0] data,
	input valid_in,
	input start,
	input[34:0] compression_length,		//length of the data before after compression (compressed data)
	input[31:0] decompression_length,	//length of the data after decompressor (uncompressed data)
	input wr_ready,

	output data_fifo_almostfull,
	
	output done,
	output last,///whether it is the last 64B of a burst
	output[511:0] data_out,
	output[63:0] byte_valid_out,
	output valid_out
);
///////parameters
parameter 	NUM_PARSER=6,  //number of Parser (2nd level parser)
			NUM_LOG=3,		//log2(NUM_PARSER), at least 1
			PARSER_MASK=6'b111111; //if set to 0, the corresponding parser will be disabled, for test only
wire[1023:0] data_w;

wire ct_page_finish;
wire dout_block_out_finish;
////////
wire df_valid,df_empty;
wire[127:0] df_dout;
wire qt_almostfull;
reg df_wr_en;
always@(*)begin
	/*valid_in is RVALID signal in axi, and fifo_almostfull is the RRREADY signal,
	the input is valid only if both signals are high*/
	if(valid_in & (~data_fifo_almostfull))begin
		df_wr_en <= 1'b1;
	end else begin
		df_wr_en <= 1'b0;
	end
end
data_fifo df0(
	.clk(clk),
	.srst(~rst_n),
	.din(data),
	.wr_en(df_wr_en),
	.rd_en((~qt_almostfull) & ~df_empty),
	.dout(df_dout),
	.almost_full(data_fifo_almostfull),
	.empty(df_empty),
	.valid(df_valid),
	.wr_rst_busy(),
	.rd_rst_busy()
);

///////////preparser
wire[143:0] pre_dout;   //18 bytes
wire[15:0] pre_tokenpos;
wire[16:0] pre_address;
wire[2:0] pre_garbage;
wire pre_startlit,pre_validout;
wire pre_page_input_finish;
preparser preparser0(
	.clk(clk),
	.data(df_dout),
	.valid(df_valid),
	.start(start),
	.compression_length_in(compression_length),
	.rst_n(rst_n),
	
	.data_out(pre_dout),
	.token_pos(pre_tokenpos),
	.address(pre_address),
	.start_lit(pre_startlit),  ///whether this 18 Byte starts with literal
	.garbage_cnt_out(pre_garbage),
	.valid_out(pre_validout),
	.page_input_finish(pre_page_input_finish)
);

/////////fifo for pages
wire[143:0] qt_dout;
wire[15:0] qt_tokenpos;
wire[16:0] qt_address;
wire[2:0] qt_garbage;
wire qt_startlit,qt_validout;
wire qt_isempty;
wire dis_rdreq;
reg qt_rdreq;
always@(*)begin
	if((qt_validout==1'b0)|dis_rdreq)begin
		qt_rdreq <=1'b1;
	end else begin
		qt_rdreq <=1'b0;
	end
end
queue_token qt0(
	.clk(clk),
	
	///////input and output of page
	.data_in(pre_dout),
	.position_in(pre_tokenpos),
	.address_in(pre_address),
	.garbage_in(pre_garbage),
	.lit_flag_in(pre_startlit),
	.wrreq(pre_validout),
	
	.data_out(qt_dout),
	.position_out(qt_tokenpos),
	.address_out(qt_address),
	.garbage_out(qt_garbage),
	.lit_flag_out(qt_startlit),
	.valid_out(qt_validout),
	////////control signal
	.rst_n(rst_n),
	
	.rdreq(qt_rdreq),
	.isempty(qt_isempty),
	
	.almost_full(qt_almostfull)
);


//////////distributor
wire[143:0] dis_dout;
wire[15:0] dis_tokenpos;
wire[16:0] dis_address;
wire[2:0] dis_garbage;
wire dis_startlit;
wire[NUM_PARSER-1:0] dis_validout;
wire dis_stop;
wire[NUM_PARSER-1:0] ps_page_req;
distributor
#(	.NUM_PARSER(NUM_PARSER)
)distributor0
(
	.clk(clk),
	.rst_n(rst_n),
	
	///////input and output of page
	.data_in(qt_dout),
	.position_in(qt_tokenpos),
	.address_in(qt_address),
	.garbage_in(qt_garbage),
	.lit_flag_in(qt_startlit),
	
	.stop(dis_stop),  ///stop the distributor
	
	.valid_in(qt_validout),
	.ready(ps_page_req&PARSER_MASK),    ///whether each parser is ready to receive new page
	
	.data_out(dis_dout),
	.position_out(dis_tokenpos),
	.address_out(dis_address),
	.garbage_out(dis_garbage),
	.lit_flag_out(dis_startlit),
	
	.rdreq(dis_rdreq),
	.valid_out(dis_validout)
);



//////generate parsers

wire[NUM_PARSER-1:0] ps_block_finish;
wire[NUM_PARSER-1:0] ps_empty;
wire[NUM_PARSER*4-1:0] ps_lit_rd;
wire[NUM_PARSER*256-1:0] ps_lit_data;
wire[NUM_PARSER*36-1:0] ps_lit_address;
wire[NUM_PARSER*32-1:0] ps_lit_wr;
wire[NUM_PARSER*16-1:0] ps_lit_ram_select;

wire[NUM_PARSER*16-1:0] ps_copy_rd;
wire[NUM_PARSER*144-1:0] ps_copy_address;
wire[NUM_PARSER*16-1:0] ps_copy_ram;
wire[NUM_PARSER*128-1:0] ps_copy_rd_out;
wire[NUM_PARSER*256-1:0] ps_offset_out;
genvar ps_i; ///i for parsers
generate 
	for(ps_i=0;ps_i<NUM_PARSER;ps_i=ps_i+1)begin: generate_parsers
		parser#(
			.PARSER_NUM(ps_i)
		)
		parser0(
			.clk(clk),
			.rst_n(rst_n),
			
			.data(dis_dout),
			.tokenpos_in(dis_tokenpos),
			.address_in(dis_address),
			.garbage_in(dis_garbage),
			.start_lit_in(dis_startlit),
	
			.valid_in(dis_validout[NUM_PARSER-1-ps_i]),
			.block_out_finish(dout_block_out_finish),  ///when 
			.page_finish(ct_page_finish),
	
			.block_finish(ps_block_finish[ps_i]),
			///for literal content 
			.lit_rd(ps_lit_rd[ps_i*4+3:ps_i*4]),
			.lit_data_out(ps_lit_data[ps_i*256+255:ps_i*256]),
			.lit_address_out(ps_lit_address[ps_i*36+35:ps_i*36]),
			.lit_wr_out(ps_lit_wr[ps_i*32+31:ps_i*32]),
			.lit_ram_select_out(ps_lit_ram_select[ps_i*16+15:ps_i*16]),
	
			///output for read token
			.copy_rd(ps_copy_rd[ps_i*16+15:ps_i*16]),
			.copy_address_out(ps_copy_address[ps_i*144+143:ps_i*144]),
			.copy_ram_select_out(ps_copy_ram[ps_i*16+15:ps_i*16]),
			.copy_rd_out(ps_copy_rd_out[ps_i*128+127:ps_i*128]),
			.copy_offset_out(ps_offset_out[ps_i*256+255:ps_i*256]),
	/////
			.all_empty(ps_empty[ps_i]),  //set to high if all the fifo are empty
			
			.ready(ps_page_req[NUM_PARSER-1-ps_i])
		);
	end
endgenerate

////////generate lit_selector, copytoken selector and ram
////////when ram_i==0, generate the ram starts with address 0
wire[64*NUM_PARSER-1:0] data_lit0,data_lit1,data_lit2,data_lit3;
wire[9*NUM_PARSER-1:0] address_lit0,address_lit1,address_lit2,address_lit3;
wire[8*NUM_PARSER-1:0] byte_valid0,byte_valid1,byte_valid2,byte_valid3;
wire[NUM_PARSER*16-1:0] ls_rd_out;
wire[NUM_PARSER*16-1:0] ls_lit_valid;
 
////generate wire for copytoken_selector
wire[33*16*NUM_PARSER-1:0] cts_data_in;
wire[16*NUM_PARSER-1:0] cts_copy_valid;
wire[16*NUM_PARSER-1:0] cts_rd_out;

genvar ram_i,ram_j,sl_i,sl_j;
generate
for(sl_i=0;sl_i<NUM_PARSER;sl_i=sl_i+1)begin: assign_wire
	assign data_lit0[64*sl_i+63:64*sl_i]	=ps_lit_data[256*sl_i+63	:256*sl_i];
	assign data_lit1[64*sl_i+63:64*sl_i]	=ps_lit_data[256*sl_i+127	:256*sl_i+64];
	assign data_lit2[64*sl_i+63:64*sl_i]	=ps_lit_data[256*sl_i+191	:256*sl_i+128];
	assign data_lit3[64*sl_i+63:64*sl_i]	=ps_lit_data[256*sl_i+255	:256*sl_i+192];

	assign address_lit0[9*sl_i+8:9*sl_i]	=ps_lit_address[36*sl_i+8:	36*sl_i];
	assign address_lit1[9*sl_i+8:9*sl_i]	=ps_lit_address[36*sl_i+17:	36*sl_i+9];
	assign address_lit2[9*sl_i+8:9*sl_i]	=ps_lit_address[36*sl_i+26:	36*sl_i+18];
	assign address_lit3[9*sl_i+8:9*sl_i]	=ps_lit_address[36*sl_i+35:	36*sl_i+27];
	
	assign byte_valid0[8*sl_i+7:8*sl_i]		=ps_lit_wr[32*sl_i+7:	32*sl_i];
	assign byte_valid1[8*sl_i+7:8*sl_i]		=ps_lit_wr[32*sl_i+15:	32*sl_i+8];
	assign byte_valid2[8*sl_i+7:8*sl_i]		=ps_lit_wr[32*sl_i+23:	32*sl_i+16];
	assign byte_valid3[8*sl_i+7:8*sl_i]		=ps_lit_wr[32*sl_i+31:	32*sl_i+24];
	
	assign ps_lit_rd[sl_i*4]	=ls_rd_out[NUM_PARSER*4*0+NUM_PARSER*0+sl_i] | ls_rd_out[NUM_PARSER*4*1+NUM_PARSER*0+sl_i] |ls_rd_out[NUM_PARSER*4*2+NUM_PARSER*0+sl_i] |ls_rd_out[NUM_PARSER*4*3+NUM_PARSER*0+sl_i];
	assign ps_lit_rd[sl_i*4+1]	=ls_rd_out[NUM_PARSER*4*0+NUM_PARSER*1+sl_i] | ls_rd_out[NUM_PARSER*4*1+NUM_PARSER*1+sl_i] |ls_rd_out[NUM_PARSER*4*2+NUM_PARSER*1+sl_i] |ls_rd_out[NUM_PARSER*4*3+NUM_PARSER*1+sl_i];
	assign ps_lit_rd[sl_i*4+2]	=ls_rd_out[NUM_PARSER*4*0+NUM_PARSER*2+sl_i] | ls_rd_out[NUM_PARSER*4*1+NUM_PARSER*2+sl_i] |ls_rd_out[NUM_PARSER*4*2+NUM_PARSER*2+sl_i] |ls_rd_out[NUM_PARSER*4*3+NUM_PARSER*2+sl_i];
	assign ps_lit_rd[sl_i*4+3]	=ls_rd_out[NUM_PARSER*4*0+NUM_PARSER*3+sl_i] | ls_rd_out[NUM_PARSER*4*1+NUM_PARSER*3+sl_i] |ls_rd_out[NUM_PARSER*4*2+NUM_PARSER*3+sl_i] |ls_rd_out[NUM_PARSER*4*3+NUM_PARSER*3+sl_i];
	
	for(sl_j=0;sl_j<16;sl_j=sl_j+1)begin: assign_wire2
		assign ls_lit_valid[sl_j*NUM_PARSER+sl_i]	=ps_lit_ram_select[sl_i*16+sl_j];
	
		//address
		assign cts_data_in[sl_j*33*NUM_PARSER+sl_i*33+32:sl_j*33*NUM_PARSER+sl_i*33+24]	=ps_copy_address[sl_i*144+sl_j*9+8:sl_i*144+sl_j*9];
		///byte select
		assign cts_data_in[sl_j*33*NUM_PARSER+sl_i*33+23:sl_j*33*NUM_PARSER+sl_i*33+16]	=ps_copy_rd_out[sl_i*128+sl_j*8+7:sl_i*128+sl_j*8];
		//offset
		assign cts_data_in[sl_j*33*NUM_PARSER+sl_i*33+15:sl_j*33*NUM_PARSER+sl_i*33+0]	=ps_offset_out[256*sl_i+16*sl_j+15:256*sl_i+16*sl_j];
	
		assign cts_copy_valid[sl_j*NUM_PARSER+sl_i] =ps_copy_ram[sl_i*16+sl_j];
		assign ps_copy_rd[sl_i*16+sl_j]             =cts_rd_out[sl_j*NUM_PARSER+sl_i];
	
	end
end
endgenerate

/************************************************
output brams
*********************************************/
wire[15:0] dout_valid_wr_in;
wire[1023:0] dout_lit_in;
wire[143:0] dout_lit_address;
wire[127:0] dout_lit_valid;

wire[511:0] dout_dout;
wire dout_cl_finish;
wire dout_page_out_finish;
data_out data_out0(
	.clk(clk),
	.rst_n(rst_n),
	
	.start(start),
	///signal for writing data
	.decompression_length(decompression_length),
	.ready(wr_ready),
	.valid_wr_in(dout_valid_wr_in),
	.lit_in(dout_lit_in),
	.lit_address(dout_lit_address),
	.lit_valid(dout_lit_valid),
	.page_finish(ct_page_finish),
	
	.block_out_finish(dout_block_out_finish),
	.page_out_finish(dout_page_out_finish),
	.cl_finish(dout_cl_finish),  ///if the clean is finished
	.last(last),
	.data_o(dout_dout),
	.byte_valid_o(byte_valid_out),
	.valid_o(valid_out)

);

/****************************************
generate module for lit_selector, copytoken_selector, block ram and copy_selector blocks
there are 16 these blocks
*****************************************/
wire[1295:0] ram_even_data,ram_odd_data;///81*16 [80:17] data  [16:8] address of ram [7:0] byte valid
wire[255:0] ram_select,crs_data_valid,crs_rd_out,ram_rd_in;
wire[15:0] ram_halffull;
wire[15:0] ram_empty;
assign dis_stop=(ram_halffull!=16'b0);
generate
for(ram_i=0;ram_i<16;ram_i=ram_i+1)begin:generate_ram
	for(ram_j=0;ram_j<16;ram_j=ram_j+1)begin:assign_ram_wire
		assign crs_data_valid[ram_i*16+ram_j] = ram_select[ram_j*16+ram_i];
		assign ram_rd_in[ram_i*16+ram_j]	  = crs_rd_out[ram_j*16+ram_i];
	end


reg[64*NUM_PARSER-1:0] data_lit_w;
reg[9*NUM_PARSER-1:0] address_lit_w;
reg[8*NUM_PARSER-1:0] byte_valid_lit_w;
always@(*)begin
	case(ram_i & 32'b11)
	32'b00:begin	data_lit_w<=data_lit0;	address_lit_w<=address_lit0; byte_valid_lit_w<=byte_valid0; end
	32'b01:begin	data_lit_w<=data_lit1;	address_lit_w<=address_lit1; byte_valid_lit_w<=byte_valid1; end
	32'b10:begin	data_lit_w<=data_lit2;	address_lit_w<=address_lit2; byte_valid_lit_w<=byte_valid2; end
	32'b11:begin	data_lit_w<=data_lit3;	address_lit_w<=address_lit3; byte_valid_lit_w<=byte_valid3; end
	default:;
	endcase
end

wire[63:0] ls_data_out;
wire[8:0] ls_address;
wire[7:0] ls_byte_valid;
wire ls_valid_out;
wire[63:0] ls_data_copy;
wire[8:0] ls_address_copy;
wire[7:0] ls_byte_valid_copy;
wire ls_copy_valid;
lit_selector#(
	.NUM_PARSER(NUM_PARSER),
	.NUM_LOG(NUM_LOG)
)lit_selector0(
	.clk(clk),
	.rst_n(rst_n),
	
	.data_lit(data_lit_w),
	.lit_address(address_lit_w),
	.byte_valid_in(byte_valid_lit_w),
	.lit_valid(ls_lit_valid[ram_i*NUM_PARSER+NUM_PARSER-1:ram_i*NUM_PARSER]),  //whether the data of parser is for this selector
	
	.data_copy(ls_data_copy),            //data from read result
	.address_copy(ls_address_copy),         //address from read result
	.byte_valid_copy(ls_byte_valid_copy),
	.copy_valid(ls_copy_valid),				 
	
	.rd_out(ls_rd_out[NUM_PARSER*ram_i+NUM_PARSER-1:NUM_PARSER*ram_i]),    ///select a parser to read
	
	.data_out(ls_data_out),
	.address_out(ls_address),
	.byte_valid_out(ls_byte_valid),
	.valid_out(ls_valid_out)
);

wire[8:0] cts_address_out;
wire[7:0] cts_bvalid_out;
wire[15:0] cts_offset_out;
wire cts_valid_out;
wire[32:0] cts_unsolved_data;
wire cts_unsolved_valid;
wire cts_unsolved_full;
wire cts_unsolved_rd;
wire cts_stop;
copytoken_selector
#(
	.NUM_PARSER(NUM_PARSER),
	.NUM_LOG(NUM_LOG)
)copy_selector0
(
	.clk(clk),
	.rst_n(rst_n),
	.data_in(cts_data_in[ram_i*33*NUM_PARSER+33*NUM_PARSER-1:ram_i*33*NUM_PARSER]), ////33*6
	.copy_valid(cts_copy_valid[ram_i*NUM_PARSER+NUM_PARSER-1:ram_i*NUM_PARSER]),
		
	.stop(cts_stop),   ///if the even fifo or odd fifo in corresponding BRAM module is almost full, stop
//	.unsolved_full(cts_unsolved_full),
	
	.unsolved_in(cts_unsolved_data), /// unsolved token is returned again
	.unsolved_valid_in(cts_unsolved_valid),  ///whether the unsolved data is valid
	
	.rd_out(cts_rd_out[ram_i*NUM_PARSER+NUM_PARSER-1:ram_i*NUM_PARSER]),  //choose which parser to read
	.unsolved_rd_out(cts_unsolved_rd),	
	
	.address_out(cts_address_out),        //address of the ram
	.bvalid_out(cts_bvalid_out),        //choose which byte to read
	.offset_out(cts_offset_out),		//offset of the byte
	.valid_out(cts_valid_out)
);

ram_module
#( .BLOCKNUM(ram_i[3:0])   ///define the number of this block, from 0 to 15
)ram0
(
	.clk(clk),
	.rst_n(rst_n),
	.rd_en(1'b1),   ///read the ram
	.empty(ram_empty[ram_i]),   ///whether all fifo are empty
	.block_out_finish(dout_block_out_finish),  //when the block output is finished
	.page_finish(ct_page_finish),  //in the end of a file, clean all data
	///signal for writing data
	.valid_wr_in(ls_valid_out),
	.lit_in(ls_data_out),
	.lit_address(ls_address),
	.lit_valid(ls_byte_valid),
	
	//signal for reading data
	.valid_rd_in(cts_valid_out),
	.copy_address(cts_address_out),
	.copy_valid_in(cts_bvalid_out),  //choose bytes to read
	.copy_offset_in(cts_offset_out),
	
	.lit_almost_full(cts_stop),    ///even or odd fifo almost full
	
	///signal for even fifo
	.even_rd(			{ram_rd_in[ram_i*16+14],ram_rd_in[ram_i*16+12],ram_rd_in[ram_i*16+10],ram_rd_in[ram_i*16+8],ram_rd_in[ram_i*16+6],ram_rd_in[ram_i*16+4],ram_rd_in[ram_i*16+2],ram_rd_in[ram_i*16+0]}!=8'b0),
	.even_data_out(ram_even_data[ram_i*81+80:ram_i*81]),
	.even_ram_select(	{ram_select[ram_i*16+14],ram_select[ram_i*16+12],ram_select[ram_i*16+10],ram_select[ram_i*16+8],ram_select[ram_i*16+6],ram_select[ram_i*16+4],ram_select[ram_i*16+2],ram_select[ram_i*16+0]}),
	
	///signal for odd fifo 
	.odd_rd(			{ram_rd_in[ram_i*16+15],ram_rd_in[ram_i*16+13],ram_rd_in[ram_i*16+11],ram_rd_in[ram_i*16+9],ram_rd_in[ram_i*16+7],ram_rd_in[ram_i*16+5],ram_rd_in[ram_i*16+3],ram_rd_in[ram_i*16+1]}!=8'b0),
	.odd_data_out(ram_odd_data[ram_i*81+80:ram_i*81]),
	.odd_ram_select(	{ram_select[ram_i*16+15],ram_select[ram_i*16+13],ram_select[ram_i*16+11],ram_select[ram_i*16+9],ram_select[ram_i*16+7],ram_select[ram_i*16+5],ram_select[ram_i*16+3],ram_select[ram_i*16+1]}),
	
	////////signal for unsolved fifo
	.unsolved_rd(cts_unsolved_rd),
	.unsolved_half_full(ram_halffull[ram_i]),
	.unsolved_data_out(cts_unsolved_data),
	.unsolved_valid_out(cts_unsolved_valid),
	/////output the decompression result 
	.data_out(data_w[64*ram_i+63:64*ram_i])

);

reg[1295:0] crs_data_in;
always@(*)begin
	if(ram_i[0]==1'b0)begin
		crs_data_in<= ram_even_data;
	end else begin
		crs_data_in<= ram_odd_data;
	end
end

copyread_selector copyread_selector0(
	.clk(clk),
	.rst_n(rst_n),
	
	.data_in(crs_data_in),   ///81*16 [80:17] data  [16:8] address of ram [7:0] byte valid
	.data_valid(crs_data_valid[ram_i*16+15:ram_i*16]),   ///whether input data is valid

	.rd_out(crs_rd_out[ram_i*16+15:ram_i*16]),
	.data_out(ls_data_copy),
	.address_out(ls_address_copy),
	.validbyte_out(ls_byte_valid_copy),
	.valid_out(ls_copy_valid)
);

assign dout_valid_wr_in[ram_i]=ls_valid_out;
assign dout_lit_in[ram_i*64+63:ram_i*64]=ls_data_out;
assign dout_lit_address[ram_i*9+8:ram_i*9]=ls_address;
assign dout_lit_valid[ram_i*8+7:ram_i*8]=ls_byte_valid;

end
endgenerate

/************************************************
control module
*********************************************/

control#
(
	.NUM_PARSER(NUM_PARSER)
)control0
(
	.clk(clk),
	.rst_n(rst_n),
	
	.tf_empty(qt_isempty), //token fifo
	.ps_finish(ps_block_finish),
	.page_input_finish(pre_page_input_finish),
	.ps_empty(ps_empty), //parser  
	.ram_empty(ram_empty),

	.cl_finish(dout_cl_finish),
	.page_finish(ct_page_finish)
	
);

assign done=dout_page_out_finish;
assign data_out=dout_dout;

endmodule 
