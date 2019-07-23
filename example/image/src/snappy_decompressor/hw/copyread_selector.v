/********************************************
File name: 		copyread_selector
Author: 		Jianyu Chen
School: 		Delft Univsersity of Technology
Date:			12th July, 2018
Description:	This is a selector to select write command for 16 BRAMs in 16 ram_module. The source of the incoming
				commands can also from the copytoken_selector (the recycle unit on the paper). The command from 
				copytoken_selector will get priority.
				Round-Robin is the stategy to do the selection if multiple ram_module have copy commands targeting this selector at the same time
				Each of this module is corresponding to one BRAM
********************************************/

`timescale 1ns/1ps

module copyread_selector(
	input clk,
	input rst_n,
	
	input[1295:0] data_in,   ///81*16 [80:72] address of ram [71:64] byte valid  [63:0] data  
	input[15:0] data_valid,   ///whether input data is valid

	output[15:0] rd_out,
	output[63:0] data_out,
	output[8:0] address_out,
	output[7:0] validbyte_out,
	output valid_out
);

reg[80:0] data_buff;
reg valid_buff;
wire[80:0] data_w;
always@(posedge clk)begin
	data_buff	<=data_w;
	valid_buff	<=(data_valid!=16'b0);   //if one of the parser gives request, it is valid
end

/////////////////////for arbiter
reg[15:0] base;
wire[15:0] grand_w;
always@(posedge clk)begin
	if(~rst_n)begin
		base	<= 16'b0000_0000_0000_0001;
	end 
	else if(rd_out==0)begin
		base	<= base;
	end
	else begin
		base	<= {rd_out[14:0],rd_out[15]};
	end
end

arbiter 
#(
	.WIDTH(16)
)arbiter0
(
	.req(data_valid),
	.grant(grand_w),
	.base(base)
);
/////////////////////////////////

function [15:0] onehot_int;
        input [15:0] in;
        integer i;
        begin
            onehot_int = 0;
            for (i = 15; i >= 0; i=i-1) begin
                if (in[i])
                    onehot_int = i;
            end
        end
endfunction

select
#(
	.NUM_SEL	(16),
	.NUM_LOG	(4),
	.NUM_WIDTH	(81)
)select0
(
	.data_in(data_in),
    .sel(onehot_int(grand_w)),
    .data_out(data_w)
);
assign rd_out       =grand_w;
assign data_out		=data_buff[63:0];
assign validbyte_out=data_buff[71:64];
assign address_out	=data_buff[80:72];
assign valid_out	=valid_buff;

endmodule 
