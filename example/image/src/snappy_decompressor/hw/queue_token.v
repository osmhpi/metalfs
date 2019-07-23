/****************************
Module name: 	queue_token
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			10th Sept, 2018
Function:		The queue (you can also call it FIFO) to store the slice from the preparser (the first level parser)
****************************/
////format of input : | 18Byte data | 16bit token position | 16bit address | 1bit flag to check whether starts with literal content |
`timescale 1ns/1ps

module queue_token(
	input clk,
	input rst_n,
	///////input and output of page
	input[143:0] data_in,
	input[15:0] position_in,
	input[16:0] address_in,
	input[2:0] garbage_in,
	input lit_flag_in,
	input wrreq,
	
	output[143:0] data_out,
	output[15:0] position_out,
	output[16:0] address_out,
	output[2:0] garbage_out,
	output lit_flag_out,
	output valid_out,
	////////control signal
	
	
	input rdreq,
	output isempty,
	
	output almost_full
);

reg valid_reg;
always@(posedge clk)begin
	if(~rst_n)begin
		valid_reg <=1'b0;
	end else if(isempty==1'b0 & valid_reg==1'b0)begin
		valid_reg <=1'b1;
	end else if(rdreq)begin
		valid_reg <= ~isempty;
	end
end

wire[180:0] q;
page_fifo pf0(
	.clk(clk),
	.srst(~rst_n),
	.din({data_in,position_in,address_in,garbage_in,lit_flag_in}),
	.wr_en(wrreq),
	.rd_en(isempty?1'b0:rdreq),
	.dout(q),
	.full(),
	.valid(),
	.empty(isempty),
	.prog_full(almost_full),
	.wr_rst_busy(),
	.rd_rst_busy()
);
assign data_out		=	q[180:37];
assign position_out	=	q[36:21];
assign address_out	=	q[20:4];
assign garbage_out	=	q[3:1];
assign lit_flag_out	=	q[0];
assign valid_out	=	valid_reg;

endmodule
