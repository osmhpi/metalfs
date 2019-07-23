/****************************
Module name: 	distributor
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			10th Sept, 2018
Function:		Get slice from queue_token module (a fifo to store the slices). And distribute the slice to 
				one of the idle parsers (2nd level parser) using Round-Robin strategy
****************************/
`timescale 1ns/1ps

module distributor
#(	parameter NUM_PARSER=6,BASE_INIT = 1
)
(
	input clk,
	input rst_n,
	
	///////input and output of page
	input[143:0] data_in,
	input[15:0] position_in,
	input[16:0] address_in,
	input[2:0] garbage_in,
	input lit_flag_in,
	
	input stop,  ///stop the distributor
	
	input valid_in,
	input[NUM_PARSER-1:0] ready,    ///whether each parser is ready to receive new page
	
	output[143:0] data_out,
	output[15:0] position_out,
	output[16:0] address_out,
	output[2:0] garbage_out,
	output lit_flag_out,
	
	output rdreq,
	output[NUM_PARSER-1:0] valid_out
);
/********for test and debug only***********/
reg[15:0] cnt_total,cnt_out;
always@(posedge clk)begin
	if(~rst_n)begin
		cnt_total<=16'b0;
	end else begin
		cnt_total<=cnt_total+16'b1;
	end
	
	if(~rst_n)begin
		cnt_out<=16'b0;
	end else if(rdreq) begin
		cnt_out<=cnt_out+16'b1;
	end
	
end
/***********************/


reg stop_reg;
always@(posedge clk)begin
	if(~rst_n)begin
		stop_reg <=1'b0;
	end else begin
		stop_reg	<=stop;
	end	
end

/////for arbiter
wire[NUM_PARSER-1:0] grand_w;
reg[NUM_PARSER-1:0] base;
arbiter 
#(
	.WIDTH(NUM_PARSER)
)arbiter0
(
	.req(ready),
	.grant(grand_w),
	.base(base)
);
always@(posedge clk)begin
	if(~rst_n)begin
		base	<= BASE_INIT;
	end 
	else if(ready==0 | (valid_in==1'b0))begin  //if no parser is ready to receive or no data in fifo
		base	<= base;
	end
	else begin
		base	<= grand_w;
	end
end

/*
////extend valid_in signal to NUM_PARSER
reg[NUM_PARSER-1:0] valid_total;
integer i;
always@(*)begin
	for(i=0;i<NUM_PARSER;i=i+1)begin:loop
		valid_total[i]	<= valid_in;
	end
end
*/

/////
assign valid_out	=stop_reg?0:(valid_in?grand_w:0);

assign data_out		=data_in;
assign position_out	=position_in;
assign address_out	=address_in;
assign lit_flag_out	=lit_flag_in;
assign garbage_out	=garbage_in;
assign rdreq		=(ready!=0) & valid_in & (~stop_reg);


endmodule
