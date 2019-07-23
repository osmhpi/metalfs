/****************************
Module name: 	copytoken_selector
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			10th Sept, 2018
Function:		Each of this selector is corresponding to one ram_module (with one BRAM inside). The incoming copy BRAM-based command
				of this module is from two kind of sources: copy command FIFOs in the parsers and the unsolved command FIFO of the 
				corresponding ram_module. If the unsolved command FIFO is not full, the command from parser will get the priority, if full,
				the data from unsolved command FIFO will get a priority.
				Round-Robin is the stategy to do the selection if multiple parsers have copy commands targeting this selector at the same time
****************************/

`timescale 1ns/1ps

module copytoken_selector
#(
	parameter NUM_PARSER=6,
	NUM_LOG=3,
	BASE_INIT=6'b1
)
(
	input clk,
	input rst_n,
	input[33*NUM_PARSER-1:0] data_in, ////33*6 [32:24]address [23:16]byte valid [15:0]offset
	input[NUM_PARSER-1:0] copy_valid,
	
	input stop,   ///if the even fifo or odd fifo in corresponding BRAM module is almost full, stop
//	input unsolved_full,
	
	input[32:0] unsolved_in, /// unsolved token is returned again
	input unsolved_valid_in,  ///whether the unsolved data is valid
	
	output[NUM_PARSER-1:0] rd_out,  //choose which parser to read
	output unsolved_rd_out,
	
	output[8:0] address_out,        //address of the ram
	output[7:0] bvalid_out,        //choose which byte to read
	output[15:0] offset_out,		//offset of the byte
	output valid_out
);


reg[32:0] data_buff;
reg valid_buff;
wire[32:0] data_w;
always@(posedge clk)begin
	data_buff	<=(copy_valid==0)?unsolved_in:data_w;
	valid_buff	<=stop?1'b0:((copy_valid!=0)|unsolved_valid_in);   //if one of the parser gives request, it is valid
end


/////////////////////for arbiter
reg[NUM_PARSER-1:0] base;
wire[NUM_PARSER-1:0] grand_w;
always@(posedge clk)begin
	if(~rst_n)begin
		base	<= BASE_INIT;
	end
	else if((rd_out==0)|stop)begin
		base	<= base;
	end
	else begin
		base	<= rd_out;
	end
end

arbiter 
#(
	.WIDTH(NUM_PARSER)
)arbiter0
(
	.req(copy_valid),
	.grant(grand_w),
	.base(base)
);
/////////////////////////////////



function [NUM_LOG-1:0] onehot_int;
        input [NUM_PARSER-1:0] in;
        integer i;
        begin
            onehot_int = 0;
            for (i = NUM_PARSER-1; i >= 0; i=i-1) begin
                if (in[i])
                    onehot_int = i;
            end
        end
endfunction

select
#(
	.NUM_SEL	(NUM_PARSER),
	.NUM_LOG	(NUM_LOG),
	.NUM_WIDTH	(33)
)select0
(
	.data_in(data_in),
    .sel(onehot_int(grand_w)),
    .data_out(data_w)
);

assign rd_out       =stop?0:grand_w;
assign unsolved_rd_out=stop?1'b0:((copy_valid==0)?unsolved_valid_in:1'b0);
assign address_out	=data_buff[32:24];
assign bvalid_out	=data_buff[23:16];
assign offset_out	=data_buff[15:0];
assign valid_out	=valid_buff;

endmodule 
