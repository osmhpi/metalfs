/****************************
Module name: 	ram_block
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			13th July, 2018
Function:		instantiate a BRAM block to store the decompression result.
				Also contains other circuit to process the write command and
				copy command.
****************************/

`timescale 1ns/1ps

module ram_block
#( parameter BLOCKNUM=4'b0   ///define the number of this block, from 0 to 15
)
(
	input clk,
	input rst_n,
	input rd_en,   ///read the ram
	input block_out_finish,  //when the page is finished
	input page_finish,  //in the end of a file, clean all data
	///signal for writing data
	input valid_wr_in,
	input[63:0] lit_in,
	input[8:0] lit_address,
	input[7:0] lit_valid,
	
	//signal for reading data
	input valid_rd_in,
	input[8:0] copy_address,
	input[7:0] copy_valid_in,  //choose bytes to read
	input[15:0] copy_offset_in,
	
	
	/////
	output unsolved_valid_out,
	output[32:0] unsolved_token_out,

	output even_valid_out,
	output[63:0] even_data_out,
	output[7:0] even_hit_out,
	output[8:0] even_address_out,

	output odd_valid_out,
	output[63:0] odd_data_out,
	output[7:0] odd_hit_out,
	output[8:0] odd_address_out,

	output[15:0] ram_select_out,
	
	////////output the compression result
	output[63:0] data_out

);

/***************************************
process write command
***************************************/
reg valid_wr_buff;
reg[63:0] lit_data_buff;
reg[8:0] lit_address_buff;
reg[7:0] lit_valid_buff;
reg[7:0] lit_byte_enable;

reg[8:0] cl_address;
reg cl_flag;
reg[2:0] cl_state;
reg valid_inverse; ///
always@(*)begin
	
	lit_data_buff	<=lit_in;
	lit_address_buff<=cl_flag?cl_address:lit_address;
	valid_wr_buff	<=cl_flag?1'b1:valid_wr_in;
	lit_valid_buff	<=cl_flag?8'b0:(lit_valid^{8{valid_inverse}});
	lit_byte_enable	<=cl_flag?8'hff:lit_valid;
end


always@(posedge clk)begin
	if(~rst_n)begin
		cl_state	<= 3'd0;
		cl_flag		<= 1'b0;
	end else
	case(cl_state)
	3'd0:begin///idle state
		cl_address	<=9'b0;	
		if(page_finish)begin
			cl_state	<=3'd1;
			cl_flag		<=1'b1;
		end
	end
	3'd1:begin  ///increasing address to clean
		cl_address	<=cl_address+9'd1;
		if(cl_address==9'd511)begin
			cl_state	<=3'd2;
			cl_flag		<=1'b0;
		end		
	end
	3'd2:begin
		cl_state	<=3'd0;
		cl_flag		<=1'b0;
	end
	default:cl_state<=3'd0;
	endcase
	
	if(~rst_n)begin
		valid_inverse<=1'b0;
	end else
	if(page_finish)begin
		valid_inverse<=1'b0;
	end else 
	if(block_out_finish)begin
		valid_inverse<=~valid_inverse;
	end
end

/***************************************
process copy command
***************************************/
wire[7:0] copy_valid_w,copy_valid_w2;
wire[63:0] copy_data_w;

//1st stage
reg valid_rd_buff;
reg[8:0] copy_address_buff;
reg[7:0] copy_valid_buff;
reg[15:0] copy_offset_buff;
always@(*)begin //if change to posedge, add reset
	valid_rd_buff		<=valid_rd_in;
	copy_address_buff	<=copy_address;
	copy_valid_buff		<=copy_valid_in;
	copy_offset_buff	<=copy_offset_in;
end

//2nd stage
reg valid_rd_buff2;
reg[8:0] copy_address_buff2;
reg[7:0] copy_valid_buff2;
reg[15:0] copy_offset_buff2;
reg[15:0] des_address2;    ///address of the destination
always@(posedge clk)begin
	if(~rst_n)begin
		valid_rd_buff2		<= 1'b0;
	end else begin
		valid_rd_buff2		<= valid_rd_buff;
		
	end
	copy_valid_buff2	<= copy_valid_buff;
	copy_address_buff2	<=copy_address_buff;
	copy_offset_buff2	<=copy_offset_buff;
	
	des_address2		<={copy_address_buff,BLOCKNUM,3'b0}+copy_offset_buff;
end

//3rd stage(fetch read result in this stage)
reg valid_rd_buff3;
reg[8:0] copy_address_buff3;
reg[7:0] copy_valid_buff3;
reg[15:0] copy_offset_buff3;
reg[15:0] des_address3;
reg[63:0] data_3;
reg[7:0] hit_3;            ///whether the bytes are read
always@(posedge clk)begin
	if(~rst_n)begin
		valid_rd_buff3	<= 1'b0;
	end else begin
		valid_rd_buff3	<= valid_rd_buff2;
		
	end
	copy_valid_buff3<= copy_valid_buff2;
	copy_address_buff3	<= copy_address_buff2;

	copy_offset_buff3	<= copy_offset_buff2;
	des_address3		<= des_address2;	
	
	
	data_3				<= copy_data_w[63:0];
	hit_3				<= copy_valid_w[7:0] & copy_valid_buff2;
end
assign copy_valid_w=copy_valid_w2^{8{valid_inverse}};

reg[10:0] debug1,debug2,debug3;////for debug only

//4th stage
reg valid_rd_buff4;
reg[8:0] copy_address_buff4;
reg[15:0] copy_offset_buff4;
reg[15:0] des_address4;
reg[32:0] unsolved_token4;   //unsolved read
reg valid_unsolved4;

reg[127:0] data_4;
reg[15:0] hit_4;
wire[127:0] data_shift;
assign data_shift	={data_3,64'b0}	>>{copy_offset_buff3[2:0],3'b0};
always@(posedge clk)begin
	if(~rst_n)begin
		valid_rd_buff4		<= 1'b0;
	end else begin
		valid_rd_buff4		<= valid_rd_buff3;
	end
	
	des_address4		<=des_address3;
	unsolved_token4		<={copy_address_buff3,hit_3^copy_valid_buff3,copy_offset_buff3};
	
	debug1<=copy_address_buff3;
	debug2<=hit_3^copy_valid_buff3;
	debug3<=copy_offset_buff3;
	
	if(~rst_n)begin
		valid_unsolved4		<= 1'b0;
	end else begin
		valid_unsolved4		<= ((hit_3^copy_valid_buff3)!=8'b0)&valid_rd_buff3;
	end
	
	data_4				<=data_shift[127:0];
	hit_4				<={hit_3,8'b0}		>>copy_offset_buff3[2:0];
end

///5th stage
reg valid_odd_5,valid_even_5;
reg[63:0] data_odd_5,data_even_5;
reg[7:0] hit_odd_5,hit_even_5;
reg[8:0] address_odd_5,address_even_5;
reg[15:0] ram_select_5;   ///select 2 rams to write, {ram15,ram14 ....ram0}
wire[31:0] ram_select_w;
wire[15:0] des_address_plus;
assign des_address_plus[15:3] = des_address4[15:3]+13'b1;
assign ram_select_w={16'b11,16'b11}<<des_address4[6:3];
always@(posedge clk)begin	
	//for odd and even output
	ram_select_5		<=ram_select_w[31:16];
	if(~rst_n)begin
		valid_even_5	<= 1'b0;
		valid_odd_5		<= 1'b0;
	end else if(des_address4[3])begin 
		valid_odd_5		<=(hit_4[15:8]!=8'b0)&valid_rd_buff4;
		valid_even_5	<=(hit_4[7:0]!=8'b0)&valid_rd_buff4;
	end else begin
		valid_even_5	<=(hit_4[15:8]!=8'b0)&valid_rd_buff4;
		valid_odd_5		<=(hit_4[7:0]!=8'b0)&valid_rd_buff4;	
	end
	
	if(des_address4[3])begin   //////////if starts with odd block
		data_odd_5		<=data_4[127:64];
		data_even_5		<=data_4[63:0];
		hit_odd_5		<=hit_4[15:8];
		hit_even_5		<=hit_4[7:0];
		address_even_5	<=des_address_plus[15:7];
	end  
	else begin
		data_even_5		<=data_4[127:64];
		data_odd_5		<=data_4[63:0];
		hit_even_5		<=hit_4[15:8];
		hit_odd_5		<=hit_4[7:0];
		address_even_5	<=des_address4[15:7];
	end
	address_odd_5		<=des_address4[15:7];
end

assign unsolved_valid_out=valid_unsolved4;
assign unsolved_token_out=unsolved_token4;

assign even_valid_out=valid_even_5;
assign even_data_out=data_even_5;
assign even_hit_out=hit_even_5;
assign even_address_out=address_even_5;

assign odd_valid_out=valid_odd_5;
assign odd_data_out=data_odd_5;
assign odd_hit_out=hit_odd_5;
assign odd_address_out=address_odd_5;

assign ram_select_out=ram_select_5;
////////////////////


///////this ram is for debug only, will be optimized
debugram debug_ram0(
	.addra(lit_address_buff),
	.clka(clk),
	.dina(lit_data_buff[63:0]),
//	.dina({lit_valid_buff,lit_data_buff}),
	.ena(valid_wr_buff),
	.wea(lit_valid_buff),
	
	.addrb(),
	.clkb(),
	.doutb(),
	.enb()
);
//////////////////////////////////////
/////read latency:1, no output register 
blockram result_ram0(
	.addra(lit_address_buff),
	.clka(clk),
	.dina({lit_valid_buff[7],lit_data_buff[63:56],lit_valid_buff[6],lit_data_buff[55:48],lit_valid_buff[5],lit_data_buff[47:40],lit_valid_buff[4],lit_data_buff[39:32],lit_valid_buff[3],lit_data_buff[31:24],lit_valid_buff[2],lit_data_buff[23:16],lit_valid_buff[1],lit_data_buff[15:8],lit_valid_buff[0],lit_data_buff[7:0]}),
	.ena(valid_wr_buff),
	.wea(lit_byte_enable),
	
	.addrb(copy_address_buff),
	.clkb(clk),
	.doutb({copy_valid_w2[7],copy_data_w[63:56],copy_valid_w2[6],copy_data_w[55:48],copy_valid_w2[5],copy_data_w[47:40],copy_valid_w2[4],copy_data_w[39:32],copy_valid_w2[3],copy_data_w[31:24],copy_valid_w2[2],copy_data_w[23:16],copy_valid_w2[1],copy_data_w[15:8],copy_valid_w2[0],copy_data_w[7:0]}),
	.enb(rd_en)
);
assign data_out=copy_data_w;
endmodule 
